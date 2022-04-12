#pragma once
#ifndef MIPT_NETWORK_SERVER_H
#define MIPT_NETWORK_SERVER_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <vector>
#include <unordered_map>
#include "../common/Entity.h"

struct ClientInfo{
  int id;
  std::array<char, 64> name;
};

class GameServer {
public:
  explicit GameServer(uint32_t port = 7777);
  ~GameServer();
  void Run();
  
private:
  void create_server(uint32_t port);
  void spawn_entities(uint32_t count);
  void spawn_user(int id);
  void update_AI();
  void check_cond();
  void wall_check();
  void add_new_client(ENetPeer* peer, const std::string& name);
  void resend_message(ENetPeer* peer, const std::string&  message, size_t read_len);
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* server;
  
  static inline int working_flag = 1;
  
  static const int MaxClientsCount = 64;
  int id_counter = 0;
  
  std::unordered_map<ENetPeer*, ClientInfo> clients;
  std::unordered_map<ENetPeer*, uint32_t> clients_entities;
  
  int height = 512;
  int weight = 1024;
  std::vector<Entity> entities;
  const float dt = 0.1;
};


#endif //MIPT_NETWORK_SERVER_H
