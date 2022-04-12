#ifndef MIPT_NETWORK_Lobby_H
#define MIPT_NETWORK_Lobby_H

#include <string>
#include <enet/enet.h>
#include <array>

class EnetLobby {
public:
  explicit EnetLobby(uint32_t port = 1234);
  ~EnetLobby();
  void Run();
  
private:
  void create_lobby(uint32_t port);
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* lobby;
  bool game_start = false;
  
  std::string server = "localhost:7777";
  
  static inline int working_flag = 1;
  
  static const int MaxClientsCount = 64;
  int id_counter = 0;
};


#endif //MIPT_NETWORK_Lobby_H
