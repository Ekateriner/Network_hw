#include "EnetServer.h"
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>

EnetServer::EnetServer(uint32_t port) {
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  if (enet_initialize () != 0)
  {
    std::cout << "Error: Cannot initialize ENet" << std::endl;
  }
  
  create_server(port);
}

EnetServer::~EnetServer() {
  enet_host_destroy(server);
  enet_deinitialize();
}

void EnetServer::create_server(uint32_t port) {
  ENetAddress address;
  address.port = port;
  address.host = ENET_HOST_ANY;
  
  server = enet_host_create (& address /* the address to bind the server host to */,
                             MaxClientsCount /* allow up to 32 clients and/or outgoing connections */,
                             2      /* allow up to 2 channels to be used, 0 and 1 */,
                             0      /* assume any amount of incoming bandwidth */,
                             0      /* assume any amount of outgoing bandwidth */);
  if (server == nullptr)
  {
    std::cout << "Error: Cannot create server" << std::endl;
  }
}

void EnetServer::add_new_client(ENetPeer* peer, const std::string& name) {
  std::array<char, 64> temp{};
  std::sprintf(temp.data(), "%x:%u", peer->address.host, peer->address.port);
  std::string client_addr = std::string(temp.data());
  ;
  if (name.length() == 0) {
    std::cout << "Warn: New client " << client_addr << " - without name" << std::endl;
    ClientInfo new_client {.id = id_counter, .name = temp};
    id_counter += 1;
    clients[peer] = new_client;
  }
  else {
    std::array<char, 64> name_array{};
    std::sprintf(name_array.data(), "%s", name.c_str());
    ClientInfo new_client{.id = id_counter, .name = name_array};
    id_counter += 1;
    clients[peer] = new_client;
  
    std::cout << "Info: New client - " << name << " (" << client_addr << ")" << std::endl;
  }
  
  for(auto [client_peer, info] : clients) {
    if (peer != client_peer) {
      ENetPacket* packet = enet_packet_create (&clients[peer],
                                               sizeof(clients[peer]),
                                               ENET_PACKET_FLAG_RELIABLE);
      
      enet_peer_send(client_peer, 0, packet);
  
      ENetPacket* packet_to_new = enet_packet_create (&clients[client_peer],
                                                      sizeof(clients[client_peer]),
                                                      ENET_PACKET_FLAG_RELIABLE);
  
      enet_peer_send(peer, 0, packet_to_new);
    }
  }
  enet_host_flush(server);
}

void EnetServer::resend_message(ENetPeer* peer, const std::string& message, size_t read_len) {
  std::array<char, 2048> text{};
  std::sprintf(text.data(), "From %s (id - %d): %s", clients[peer].name.data(), clients[peer].id, message.c_str());
  std::cout << "Info: " << std::string(text.data()) << std::endl;
  for(auto [client_peer, info] : clients) {
    if (peer != client_peer) {
      ENetPacket * packet = enet_packet_create (text.data(),
                                                std::string(text.data()).length(),
                                                ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
  
      enet_peer_send (client_peer, 1, packet);
    }
  }
  enet_host_flush(server);
}

void EnetServer::Run() {
  ENetEvent event;
  while(working_flag) {
    while (enet_host_service(server, &event, 1000) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
          std::string message = std::string(reinterpret_cast<char *>(event.packet->data));
          if (clients.find(event.peer) == clients.end())
            add_new_client(event.peer, message);
          else {
            resend_message(event.peer, message, event.packet->dataLength);
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT: {
          std::array<char, 64> temp{};
          std::sprintf(temp.data(), "%x:%u", event.peer->address.host, event.peer->address.port);
          std::string client_addr = std::string(temp.data());
          clients.erase(event.peer);
          std::cout << "Info: " << client_addr << " has disconnected" << std::endl;
        }
        case ENET_EVENT_TYPE_NONE:
          break;
      }
    }
  }
}