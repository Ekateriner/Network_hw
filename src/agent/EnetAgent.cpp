#include "EnetAgent.h"
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>
#include "../common/Packet.h"

EnetAgent::EnetAgent(uint32_t port) {
  struct sigaction action_term = {};
  action_term.sa_handler = handle_sigterm;
  action_term.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action_term, NULL);
  
  if (enet_initialize () != 0)
  {
    std::cout << "Error: Cannot initialize ENet" << std::endl;
  }
  
  create_agent(port);
}

EnetAgent::~EnetAgent() {
  enet_host_destroy(agent);
  enet_deinitialize();
}

void EnetAgent::create_agent(uint32_t port) {
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  
  agent = enet_host_create (& address /* the address to bind the Lobby host to */,
                            MaxClientsCount /* allow up to 32 clients and/or outgoing connections */,
                            2      /* allow up to 2 channels to be used, 0 and 1 */,
                            0      /* assume any amount of incoming bandwidth */,
                            0      /* assume any amount of outgoing bandwidth */);
  if (agent == nullptr)
  {
    std::cout << "Error: Cannot create Agent" << std::endl;
  }
}

void EnetAgent::Run() {
  ENetEvent event;
  while(working_flag) {
    while (enet_host_service(agent, &event, 1000) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          break;
        case ENET_EVENT_TYPE_RECEIVE: {
          Header head = *reinterpret_cast<Header*>(event.packet->data);
          if (head == Header::CreateServer) {
            RoomInfo info = reinterpret_cast<Packet<RoomInfo>*>(event.packet->data)->value;
            std::string args = "";
            args.append(std::to_string(min_port));
            args.append(" ");
            args.append(std::to_string(info.start_radius));
            args.append(" ");
            args.append(std::to_string(info.velocity));
            args.append(" &");
#ifdef WIN32
            std::system(std::string("../server/network_server.exe ").append(args).c_str());
#else
            std::system(std::string("../server/network_server ").append(args).c_str());
#endif
            Packet<uint> data = {.header = ServerPort, .value=min_port};
            ENetPacket *packet = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
  
            enet_peer_send(event.peer, 0, packet);
            std::cout << "Info: Send port " << min_port << std::endl;
            min_port += 1;
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
        {
          std::array<char, 64> temp{};
          std::sprintf(temp.data(), "%x:%u", event.peer->address.host, event.peer->address.port);
          std::string client_addr = std::string(temp.data());
          std::cout << "Info: " << client_addr << " has disconnected" << std::endl;
          event.peer -> data = NULL;
        }
          break;
    
        case ENET_EVENT_TYPE_NONE:
          break;
      }
    }
  }
}