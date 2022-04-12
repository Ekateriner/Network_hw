#pragma once
#ifndef MIPT_NETWORK_CLIENT_H
#define MIPT_NETWORK_CLIENT_H

#include <enet/enet.h>
#include <string>
#include <array>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <iostream>


#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "../common/Entity.h"

struct ClientInfo{
  int id;
  std::array<char, 64> name;
};

class GameClient {
public:
  explicit GameClient(std::string name = "", std::string lobby_name = "127.0.0.1", uint32_t port = 1234);
  ~GameClient();
  void Run();
  
private:
  void create_peer();
  void send_message(const std::string& message, ENetPeer* peer);
  void send_info(ENetPeer* peer, std::pair<float, float> mouse_pos);
  void draw(const std::vector<Entity>& entities);
  void init_allegro();
  
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
  
  int id = -1;
  std::pair<float, float> mouse_pos = std::make_pair(0, 0);
  std::unordered_map<int, std::string> others;
  
  const int keep_alive_duration_sec = 60; // in sec
  
  ALLEGRO_TIMER* a_timer;
  ALLEGRO_EVENT_QUEUE* a_events;
  ALLEGRO_DISPLAY* a_display;
  ALLEGRO_FONT* a_font;
  
  int weight;
  int height;
  bool redraw = true;
};


#endif //MIPT_NETWORK_CLIENT_H
