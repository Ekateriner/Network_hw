#pragma once
#ifndef MIPT_NETWORK_SERVER_H
#define MIPT_NETWORK_SERVER_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <vector>
#include <unordered_map>
#include "../common/Entity.h"
#include "../common/Room.h"
#include "../common/Packet.h"

class GameServer {
public:
  explicit GameServer(uint32_t port = 7777, float _radius = 20.0f, float _velocity=10.0f);
  ~GameServer();
  void Run();
  
private:
  void create_server(uint32_t port);
  void spawn_entities(uint32_t count);
  void spawn_user(int id);
  void update_AI();
  void check_cond();
  void wall_check();
  void add_new_client(ENetPeer* peer, const ClientInfo& info);
  void resend_message(ENetPeer* peer, const std::string&  message);
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* server;
  
  static inline int working_flag = 1;
  
  static const int MaxClientsCount = 64;
  
  std::unordered_map<ENetPeer*, ClientInfo> clients;
  std::unordered_map<ENetPeer*, uint32_t> clients_entities;
  
  int height = 512;
  int weight = 1024;
  std::vector<Entity> entities;
  const float dt = 0.1;
  
  float radius;
  float velocity;
};


#endif //MIPT_NETWORK_SERVER_H
