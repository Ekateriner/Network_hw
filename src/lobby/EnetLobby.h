#ifndef MIPT_NETWORK_Lobby_H
#define MIPT_NETWORK_Lobby_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <vector>
#include <queue>
#include <experimental/array>
#include "../common/Room.h"
#include "../common/Packet.h"

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
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* lobby;
  uint start_level = 3;
  
  std::array<Agent, 1> agents = {Agent{.name="localhost", .port=7777}};
  
  static inline int working_flag = 1;
  
  static const int MaxClientsCount = 64;
  uint id_counter = 0;
  
  std::vector<Room> rooms;
  Room wait_room = Room("wait", -1, 0, 0);
};


#endif //MIPT_NETWORK_Lobby_H
