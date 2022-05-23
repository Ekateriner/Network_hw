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
#include "../common/Room.h"

class GameClient {
public:
  explicit GameClient(std::string name = "", std::string lobby_name = "127.0.0.1", uint32_t port = 1234);
  ~GameClient();
  void Run();
  
private:
  void create_peer();
  void send_message(const std::string& message);
  void send_info(std::pair<float, float> mouse_pos);
  void draw(const std::unordered_map<uint, Entity>& entities);
  void init_allegro();
  void process_event(ENetEvent& event);
  void clean_game();
  
  static std::string console_input(){
    std::string answer;
    std::getline(std::cin, answer);
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
  int room_id = -1;
  
  int id = -1;
  std::unordered_map<int, std::string> others;
  bool creation_stage = false;
  
  const int keep_alive_duration_sec = 60; // in sec
  
  ALLEGRO_TIMER* a_timer;
  ALLEGRO_EVENT_QUEUE* a_events;
  ALLEGRO_DISPLAY* a_display;
  ALLEGRO_FONT* a_font;
  
  int weight;
  int height;
  std::unordered_map<uint, Entity> state;
  bool redraw = true;
  
  int lobby_key = 0;
  int server_key = 0;
};


#endif //MIPT_NETWORK_CLIENT_H
