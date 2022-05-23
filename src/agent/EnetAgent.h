#ifndef MIPT_NETWORK_Lobby_H
#define MIPT_NETWORK_Lobby_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <vector>
#include "../common/Room.h"

class EnetAgent {
public:
  explicit EnetAgent(uint32_t port = 7777);
  ~EnetAgent();
  void Run();
  
private:
  void create_agent(uint32_t port);
  void new_server();
  
  static void handle_sigterm(int signum) {
    working_flag = 0;
  };
  
  ENetHost* agent;
  uint min_port = 9000;
  
  static const int MaxClientsCount = 64;
  static inline int working_flag = 1;
};


#endif //MIPT_NETWORK_Lobby_H
