#ifndef MIPT_NETWORK_CLIENT_H
#define MIPT_NETWORK_CLIENT_H

#include <enet/enet.h>
#include <string>
#include <array>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <iostream>

struct ClientInfo{
  int id;
  std::array<char, 64> name;
};

class EnetClient {
public:
  explicit EnetClient(std::string name = "", std::string lobby_name = "127.0.0.1", uint32_t port = 1234);
  ~EnetClient();
  void Run();
  
private:
  void create_peer();
  void send_message(const std::string& message, ENetPeer* peer);
  static std::string console_input(){
    std::string answer;
    std::cin >> answer;
    return answer;
  }
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* client;
  ENetPeer* lobby_peer;
  ENetPeer* server_peer;
  
  std::string lobby_name;
  uint32_t lobby_port;
  
  std::string name;
  static inline int working_flag = 1;
  bool in_game = false;
  
  std::vector<ClientInfo> others;
  
  const int keep_alive_duration_sec = 60; // in sec
};


#endif //MIPT_NETWORK_CLIENT_H
