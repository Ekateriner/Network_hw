#include "EnetLobby.h"
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>

EnetLobby::EnetLobby(uint32_t port) {
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  if (enet_initialize () != 0)
  {
    std::cout << "Error: Cannot initialize ENet" << std::endl;
  }
  
  create_lobby(port);
}

EnetLobby::~EnetLobby() {
  enet_host_destroy(lobby);
  enet_deinitialize();
}

void EnetLobby::create_lobby(uint32_t port) {
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  
  lobby = enet_host_create (& address /* the address to bind the Lobby host to */,
                             MaxClientsCount /* allow up to 32 clients and/or outgoing connections */,
                             2      /* allow up to 2 channels to be used, 0 and 1 */,
                             0      /* assume any amount of incoming bandwidth */,
                             0      /* assume any amount of outgoing bandwidth */);
  if (lobby == nullptr)
  {
    std::cout << "Error: Cannot create Lobby" << std::endl;
  }
}

void EnetLobby::Run() {
  ENetEvent event;
  while(working_flag) {
    while (enet_host_service(lobby, &event, 1000) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          if (game_start) {
            ENetPacket *packet = enet_packet_create(server.c_str(),
                                                    server.length(),
                                                    ENET_PACKET_FLAG_RELIABLE);
        
            enet_peer_send(event.peer, 0, packet);
            enet_host_flush(lobby);
          }
        }
          break;
        case ENET_EVENT_TYPE_RECEIVE: {
          std::string message = std::string(reinterpret_cast<char *>(event.packet->data));
          if (message == "start") {
            game_start = true;
            ENetPacket *packet = enet_packet_create(server.c_str(),
                                                    server.length(),
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(lobby, 0, packet);
            enet_host_flush(lobby);
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
          break;
    
        case ENET_EVENT_TYPE_NONE:
          break;
      }
    }
  }
}