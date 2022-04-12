#ifndef MIPT_NETWORK_SERVER_H
#define MIPT_NETWORK_SERVER_H

#include <string>
#include <enet/enet.h>
#include <array>
#include <unordered_map>

struct ClientInfo{
  int id;
  std::array<char, 64> name;
};

class EnetServer {
public:
  explicit EnetServer(uint32_t port = 7777);
  ~EnetServer();
  void Run();
  
private:
  void create_server(uint32_t port);
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
};


#endif //MIPT_NETWORK_SERVER_H
