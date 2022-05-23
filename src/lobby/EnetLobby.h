#ifndef MIPT_NETWORK_Lobby_H
#define MIPT_NETWORK_Lobby_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <limits>

#include "../common/Room.h"

struct Agent {
  std::string name;
  uint32_t port;
  ENetPeer* peer;
  
  std::queue<uint> rooms_wait;
};

class EnetLobby {
public:
  explicit EnetLobby(uint32_t port = 1234);
  ~EnetLobby();
  void Run();
  
private:
  void create_lobby(uint32_t port);
  void send_rooms_list(ENetPeer* receiver);
  void start_room(uint32_t id);
  void process_event(ENetEvent& event);
  static int generate_key() {
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> rand_key(INT32_MIN, INT32_MAX);
    return rand_key(gen);
  }
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* lobby;
  uint start_level = 3;
  
  std::array<Agent, 1> agents = {Agent{.name="localhost", .port=7777}};
  std::unordered_map<ENetPeer*, int> clients_key;
  
  static inline int working_flag = 1;
  
  static const int MaxClientsCount = 64;
  uint id_counter = 0;
  
  std::vector<Room> rooms;
  Room wait_room = Room("wait", -1, 0, 0);
};


#endif //MIPT_NETWORK_Lobby_H
