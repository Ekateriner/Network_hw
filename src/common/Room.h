#ifndef MIPT_NETWORK_ROOM_H
#define MIPT_NETWORK_ROOM_H

#include <string>
#include <utility>
#include <unordered_map>
#include <enet/enet.h>

struct ClientInfo{
  int id;
  std::array<char, 64> name{};
};

struct RoomInfo {
  std::array<char, 64> name{};
  bool game_start;
  
  uint32_t clients_count;
  
  uint32_t max_clients_count;
  uint start_radius;
  uint velocity;
  bool adaptive_vel;
  
  RoomInfo(const std::string& _name, bool start, uint _count, uint _max_count, uint _rad, uint _vel, bool adapt) :
    game_start(start),
    clients_count(_count),
    max_clients_count(_max_count),
    start_radius(_rad),
    velocity(_vel),
    adaptive_vel(adapt) {
    std::copy_n(_name.c_str(), _name.length(), name.data());
  }
  
//  RoomInfo(std::array<char, 64> _name, bool start, uint _count, uint _max_count, uint _rad, uint _vel, bool adapt) :
//    name(_name),
//    game_start(start),
//    clients_count(_count),
//    max_clients_count(_max_count),
//    start_radius(_rad),
//    velocity(_vel),
//    adaptive_vel(adapt) {}
    
  RoomInfo() {
    game_start = false;
    clients_count = 0;
    max_clients_count = 0;
    start_radius = 0;
    velocity = 0;
    adaptive_vel = false;
  }
};

struct Room {
  std::string name;
  bool game_start = false;
  
  std::unordered_map<ENetPeer*, ClientInfo> clients;
  std::string server = "localhost:";
  
  uint32_t max_clients_count;
  uint start_radius;
  uint velocity;
  bool adaptive_vel;
  
  Room(std::string _name, uint _count, uint _rad, uint _vel, bool adapt = false) :
    name(std::move(_name)),
    max_clients_count(_count),
    start_radius(_rad),
    velocity(_vel),
    adaptive_vel(adapt) {}
    
  Room(std::array<char, 64> _name, uint _count, uint _rad, uint _vel, bool adapt = false) :
    name(_name.data()),
    max_clients_count(_count),
    start_radius(_rad),
    velocity(_vel),
    adaptive_vel(adapt) {}
};

#endif //MIPT_NETWORK_ROOM_H
