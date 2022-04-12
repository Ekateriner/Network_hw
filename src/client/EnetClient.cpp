#include "EnetClient.h"
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <future>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

EnetClient::EnetClient(std::string name, std::string lobby_name, uint32_t port) :
  name(name),
  lobby_name(lobby_name),
  lobby_port(port)
{
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  if (enet_initialize () != 0)
  {
    std::cout << "Error: Cannot initialize ENet" << std::endl;
  }
  create_peer();
}

EnetClient::~EnetClient() {
  enet_host_destroy(client);
  enet_deinitialize();
}

void EnetClient::create_peer() {
  client = enet_host_create (NULL /* create a client host */,
                            2 /* only allow 1 outgoing connection */,
                            2 /* allow up 2 channels to be used, 0 and 1 */,
                            0 /* assume any amount of incoming bandwidth */,
                            0 /* assume any amount of outgoing bandwidth */);
  if (client == nullptr)
  {
    std::cout << "Error: Cannot create client" << std::endl;
  }
}

void EnetClient::send_message(const std::string& message, ENetPeer* peer) {
  ENetPacket* packet = enet_packet_create (message.c_str(),
                                           message.length(),
                                           ENET_PACKET_FLAG_RELIABLE);
  
  enet_peer_send(peer, 0, packet);
}

void EnetClient::Run() {
  ENetEvent event;
  {
    ENetAddress address;
    
    enet_address_set_host (&address, lobby_name.c_str());
    address.port = lobby_port;

    lobby_peer = enet_host_connect (client, & address, 2, 0);
    if (lobby_peer == nullptr)
    {
      std::cout << "Error: No available peers for ENet connection" << std::endl;
    }
    
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (client, & event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
      std::cout << "Info: Connected to lobby" << std::endl;
    }
    else
    {
      enet_peer_reset(lobby_peer);
      std::cout << "Error: Connection to lobby failed" << std::endl;
    }
  }
  
  auto input = std::async(std::launch::async, console_input);
  
  while(working_flag) {
    if (input.wait_for(0ms) == std::future_status::ready) {
      auto input_string = input.get();
      input = std::async(std::launch::async, console_input);
      if (input_string == "/start" && !in_game) {
        send_message("start", lobby_peer);
      }
      else if (input_string == "/other" && in_game) {
        for (auto client_info : others) {
          std::cout << "Id: " << client_info.id << " / Name: " << std::string(client_info.name.data()) << std::endl;
        }
      }
      else if (in_game) {
        send_message(input_string, server_peer);
      }
    }
    
    while ( enet_host_service(client, &event, 10) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          std::cout << "Error: Invalid client operation" << std::endl;
          break;
    
        case ENET_EVENT_TYPE_RECEIVE: {
          std::string message = std::string(reinterpret_cast<char*>(event.packet->data));
          if (!in_game) {
            uint del = message.find(':');
        
            ENetAddress address;
        
            enet_address_set_host(&address, message.substr(0, del).c_str());
            address.port = stoul(message.substr(del + 1));
        
            server_peer = enet_host_connect(client, &address, 2, 0);
            if (server_peer == nullptr) {
              std::cout << "Error: No available peers for ENet connection" << std::endl;
            }
        
            /* Wait up to 5 seconds for the connection attempt to succeed. */
            if (enet_host_service(client, &event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT) {
              std::cout << "Info: Connected to server" << std::endl;
            } else {
              enet_peer_reset(lobby_peer);
              std::cout << "Error: Connection to server failed" << std::endl;
            }
            in_game = true;
  
            send_message(name, server_peer);
        
          } else if (event.channelID == 0) {
            auto other = reinterpret_cast<ClientInfo*>(event.packet->data);
            others.push_back(*other);
          }
          else {
            std::cout << "Message: " << message << std::endl;
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
          std::cout << "Error: Invalid client operation" << std::endl;
          break;
    
        case ENET_EVENT_TYPE_NONE:
          break;
      }
    }
  }
}