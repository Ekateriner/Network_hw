#ifndef MIPT_NETWORK_CLIENT_H
#define MIPT_NETWORK_SERVER_H

#include <string>
#include <sys/epoll.h>
#include <array>
#include <unordered_map>
#include <chrono>

struct ClientInfo{
  std::string name;
  //std::string address; //"ip:port"
  std::chrono::time_point<std::chrono::system_clock> timer;
  //int fd;
};

class Server {
public:
  explicit Server(std::string host_name = "127.0.0.1", std::string port = "7777");
  void CreateEvent(int fd);
  void DeleteEvent(int fd);
  void Run();
  
private:
  void socket_create_bind_local();
  void make_socket_non_blocking();
  void add_new_client(const std::string& client_addr, std::string  message, size_t read_len);
  void resend_message(const std::string& client_addr, const std::string&  message, size_t read_len);
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  std::string host_name;
  std::string port;
  int socket_fd = 0;
  int epoll_fd = 0;
  static inline int working_flag = 1;
  
  static const int MaxPendingEvents = 10000;
  const int ListenBacklog = 50;
  const int keep_alive_duration_sec = 60; // in sec
  
  std::array<epoll_event, MaxPendingEvents> events;
  std::unordered_map<std::string, ClientInfo> clients;
};


#endif //MIPT_NETWORK_CLIENT_H
