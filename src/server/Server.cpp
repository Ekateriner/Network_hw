#include "Server.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <arpa/inet.h>
#include <sys/timerfd.h>

Server::Server(std::string host_name, std::string port) :
  host_name(std::move(host_name)),
  port(std::move(port))
{
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  socket_create_bind_local();
  //listen(socket_fd, ListenBacklog);
  make_socket_non_blocking();
  epoll_fd = epoll_create1(0);
}

void Server::socket_create_bind_local() {
  addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  
  addrinfo *result, *rp;
  if (int error = getaddrinfo(host_name.data(), port.data(), &hints, &result)) {
    printf("%s\n", gai_strerror(error));
  }
  
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    socket_fd = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
    if (socket_fd == -1)
      continue;
    
    if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;                  /* Success */
    
    close(socket_fd);
  }
  
  if (rp == nullptr) {               /* No address succeeded */
    fprintf(stderr, "Could not bind\n");
    exit(EXIT_FAILURE);
  }
  
  freeaddrinfo(result);           /* No longer needed */
}

void Server::make_socket_non_blocking() {
  int flags = fcntl(socket_fd, F_GETFD);
  fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::CreateEvent(int event_fd) {
  epoll_event event_ready_read{};
  event_ready_read.events = EPOLLIN | EPOLLET;
  event_ready_read.data.fd = event_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &event_ready_read);
}

void Server::DeleteEvent(int event_fd) {
  epoll_event event_ready_read{};
  event_ready_read.events = EPOLLIN;
  event_ready_read.data.fd = event_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, &event_ready_read);
}

void Server::add_new_client(const std::string& client_addr, std::string message, size_t read_len) {
  if (message[0] != 'n') {
    std::cout << "Error: Wrong format from new client " << client_addr << std::endl;
    return;
  }
  if (read_len == 1) {
    std::cout << "Warn: New client " << client_addr << " - without name" << std::endl;
    ClientInfo new_client {.name = client_addr, /*.address = client_addr,*/ .timer = std::chrono::high_resolution_clock::now()};
    clients[client_addr] = new_client;
  }
  else {
    ClientInfo new_client{.name = message.substr(1, read_len - 1), /*.address = client_addr,*/
                      .timer = std::chrono::high_resolution_clock::now()};
    clients[client_addr] = new_client;
  
    std::cout << "Info: New client - " << message.substr(1, read_len - 1) << " (" << client_addr << ")" << std::endl;
  }
}

void Server::resend_message(const std::string& client_addr, const std::string& message, size_t read_len) {
  std::array<char, 2048> text;
  std::sprintf(text.data(), "From %s(%s): %s",clients[client_addr].name.c_str(), client_addr.c_str(), message.substr(1, read_len-1).c_str());
  std::cout << "Info: " << std::string(text.data()) << std::endl;
  for(auto [addr, info] : clients) {
    if (addr != client_addr) {
      uint del = addr.find(':');
      sockaddr_in address{};
      in_addr inet_addr{};
      inet_aton(addr.substr(0, del).c_str(), &inet_addr);
      address.sin_family = AF_INET;
      address.sin_addr = inet_addr;
      address.sin_port = ntohs(stoul(addr.substr(del + 1)));
      
      sendto(socket_fd, text.data(), std::string(text.data()).length(), 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    }
  }
}

void Server::Run() {
  CreateEvent(socket_fd);
  timespec time{};
  time.tv_sec = keep_alive_duration_sec / 2;
  time.tv_nsec = 0;
  
  itimerspec spec{};
  spec.it_value = time;
  spec.it_interval = time;
  
  auto timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  timerfd_settime(timer_fd, 0, &spec, nullptr);
  
  CreateEvent(timer_fd);
  
  while(working_flag) {
    int event_count;
    event_count = epoll_wait(epoll_fd, events.data(), MaxPendingEvents, -1);
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == socket_fd) {
        // receive
        std::array<char, 2048> message{};
        sockaddr client_info = {};
        socklen_t addr_len = sizeof(client_info);
        auto read_len = recvfrom(events[i].data.fd, message.data(), message.size(), 0, &client_info, &addr_len);
        std::array<char, 64> temp{};
        std::sprintf(temp.data(), "%s:%d",
                     inet_ntoa(reinterpret_cast<sockaddr_in*>(&client_info)->sin_addr),
                     ntohs(reinterpret_cast<sockaddr_in*>(&client_info)->sin_port));
        std::string client_addr = std::string (temp.data());
        if (read_len == -1) {
          std::cout << "Error: Receive from " << client_addr <<  " failed!" << std::endl;
        }
        else if (read_len == 0) {
          std::cout << "Warning: Empty message from " << client_addr << std::endl;
        }
        else if (clients.find(client_addr) == clients.end())
          add_new_client(client_addr, std::string(message.data(), read_len), read_len);
        else if (message[0] == 'a') {
          //keep alive
          clients[client_addr].timer = std::chrono::high_resolution_clock::now();
        }
        else if (message[0] == 'm') {
          //new message
          clients[client_addr].timer = std::chrono::high_resolution_clock::now();
          resend_message(client_addr, std::string(message.data(), read_len), read_len);
        }
      }
      else if (events[i].data.fd == timer_fd) {
        uint64_t count;
        read(events[i].data.fd, &count, sizeof(count));
  
        //check clients for alive
        for(auto it = clients.begin(); it != clients.end(); ) {
          auto dur = std::chrono::high_resolution_clock::now() - it->second.timer;
          if (dur > std::chrono::seconds(keep_alive_duration_sec)) {
            it = clients.erase(it);
            std::cout << "Info: " << it->first << " has disconnected" << std::endl;
          }
          else {
            it++;
          }
        }
      }
      else {
        std::cout << "Error: Unknown event" << std::endl;
      }
    }
  }
  
  close(socket_fd);
  close(epoll_fd);
}