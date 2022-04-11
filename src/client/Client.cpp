#include "Client.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <arpa/inet.h>
#include <sys/timerfd.h>

Client::Client(std::string name, std::string server_name, std::string port) :
  name(name),
  server_name(server_name),
  port(port)
{
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  socket_create_connect();
  make_socket_non_blocking();
  epoll_fd = epoll_create1(0);
  
  std::sprintf(server_addr.data(), "%s:%s",
               server_name.c_str(), port.c_str());
}

void Client::socket_create_connect() {
  addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_socktype = SOCK_DGRAM;
  
  addrinfo *result, *rp;
  if (int error = getaddrinfo(server_name.data(), port.data(), &hints, &result)) {
    printf("%s\n", gai_strerror(error));
  }
  
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    socket_fd = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
    if (socket_fd == -1)
      continue;
    
    if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;                  /* Success */
    
    close(socket_fd);
  }
  
  if (rp == nullptr) {               /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }
  
  freeaddrinfo(result);           /* No longer needed */
}

void Client::make_socket_non_blocking() {
  int flags = fcntl(socket_fd, F_GETFD);
  fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

void Client::CreateEvent(int event_fd, uint32_t flag) {
  epoll_event event_ready_read;
  event_ready_read.events = flag | EPOLLET;
  event_ready_read.data.fd = event_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &event_ready_read);
}

void Client::DeleteEvent(int event_fd,  uint32_t flag) {
  epoll_event event_ready_read;
  event_ready_read.events = flag;
  event_ready_read.data.fd = event_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, &event_ready_read);
}

void Client::send_message(std::string message, size_t read_len) {

}

void Client::Run() {
  {
    std::array<char, 100> text;
    std::sprintf(text.data(), "n%s", name.c_str());
  
    sockaddr_in address;
    in_addr inet_addr;
    inet_aton(server_name.c_str(), &inet_addr);
    address.sin_family = AF_INET;
    address.sin_addr = inet_addr;
    address.sin_port = ntohs(stoul(port));
  
    sendto(socket_fd, text.data(), name.length() + 1, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
  }
  //CreateEvent(socket_fd, EPOLLOUT);
  CreateEvent(socket_fd, EPOLLIN);
  
  timespec time;
  time.tv_sec = keep_alive_duration_sec / 2;
  time.tv_nsec = 0;
  
  itimerspec spec;
  spec.it_value = time;
  spec.it_interval = time;
  
  auto timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  timerfd_settime(timer_fd, 0, &spec, nullptr);
  
  CreateEvent(timer_fd, EPOLLIN);
  CreateEvent(0, EPOLLIN);
  
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
        std::array<char, 64> client_addr;
        std::sprintf(client_addr.data(), "%s:%d",
                     inet_ntoa(reinterpret_cast<sockaddr_in*>(&client_info)->sin_addr),
                     ntohs(reinterpret_cast<sockaddr_in*>(&client_info)->sin_port));
        
        if (read_len == -1) {
          std::cout << "Error: Receive from " << std::string(client_addr.data()) <<  " failed!" << std::endl;
        }
        else if (client_addr != server_addr) {
          std::cout << "Error: Receive message from " << std::string(client_addr.data()) << ". It's not a server." << std::endl;
        }
        else if (read_len == 0) {
          std::cout << "Warning: Empty message from " << std::string(client_addr.data()) << std::endl;
        }
        else {
          std::cout << "Message: " << std::string_view(message.data(), read_len) << std::endl;
        }
      }
      else if (events[i].data.fd == timer_fd) {
        uint64_t count;
        read(events[i].data.fd, &count, sizeof(count));
        
        //send keep alive
        sockaddr_in address;
        in_addr inet_addr;
        inet_aton(server_name.c_str(), &inet_addr);
        address.sin_family = AF_INET;
        address.sin_addr = inet_addr;
        address.sin_port = ntohs(stoul(port));
  
        sendto(socket_fd, "a", 1, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
      }
      else if (events[i].data.fd == 0) {
        std::array<char, 2048> message{};
        auto read_len = read(events[i].data.fd, message.data(), message.size());
  
        std::array<char, 2048> text;
        std::sprintf(text.data(), "m%s", std::string_view(message.data(), read_len).data());
        
        sockaddr_in address;
        in_addr inet_addr;
        inet_aton(server_name.c_str(), &inet_addr);
        address.sin_family = AF_INET;
        address.sin_addr = inet_addr;
        address.sin_port = ntohs(stoul(port));
  
        sendto(socket_fd, text.data(), read_len + 1, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
      }
      else {
        std::cout << "Error: Unknown event" << std::endl;
      }
    }
  }
  
  close(socket_fd);
  close(epoll_fd);
}