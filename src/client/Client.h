#ifndef MIPT_NETWORK_CLIENT_H
#define MIPT_NETWORK_CLIENT_H

#include <string>
#include <sys/epoll.h>
#include <array>
#include <unordered_map>
#include <chrono>

class Client {
public:
  explicit Client(std::string name = "", std::string server_name = "127.0.0.1", std::string port = "7777");
  void CreateEvent(int fd, uint32_t flag);
  void DeleteEvent(int fd, uint32_t flag);
  void Run();
  
private:
  void socket_create_connect();
  void make_socket_non_blocking();
  void send_message(std::string message, size_t read_len);
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  std::string server_name;
  std::string port;
  std::array<char, 64> server_addr;
  std::string name;
  int socket_fd;
  int epoll_fd;
  static inline int working_flag = 1;
  
  static const int MaxPendingEvents = 10000;
  const int ListenBacklog = 50;
  const int keep_alive_duration_sec = 60; // in sec
  
  std::array<epoll_event, MaxPendingEvents> events;
};


#endif //MIPT_NETWORK_CLIENT_H
