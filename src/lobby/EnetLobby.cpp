#include "EnetLobby.h"
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>
#include "../common/Packet.h"

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

void EnetLobby::send_rooms_list(ENetPeer* receiver) {
  //std::cout << "Info: Room list send" << std::endl;
  std::vector<RoomInfo> roomInfos;
  for(auto& room: rooms){
    roomInfos.emplace_back(room.name, room.game_start, room.clients.size(), room.max_clients_count,
                           room.start_radius, room.velocity, room.adaptive_vel);
  }
  
  std::array<RoomInfo, 256> rooms_data{};
  std::copy_n(roomInfos.data(), roomInfos.size(), rooms_data.data());
  Packet<std::array<RoomInfo, 256>> data = {.header = Header::RoomList, .value = rooms_data};
  subscribe(&data, clients_key[receiver], sizeof(data));
  ENetPacket *packet = enet_packet_create(&data,
                                          sizeof(data),
                                          ENET_PACKET_FLAG_RELIABLE);
  
  enet_peer_send(receiver, 0, packet);
  enet_host_flush(lobby);
}

void EnetLobby::start_room(uint32_t id) {
  Room room = rooms[id];
  Packet<RoomInfo> data = {.header = Header::CreateServer, .value=RoomInfo(room.name, room.game_start, room.clients.size(), room.max_clients_count,
                                                                           room.start_radius, room.velocity, room.adaptive_vel)};
  ///!!!!!!agent??????
  ENetPacket *packet = enet_packet_create(&data,
                                          sizeof(data),
                                          ENET_PACKET_FLAG_RELIABLE);
  
  enet_peer_send(agents[0].peer, 0, packet);
  agents[0].rooms_wait.push(id);
  enet_host_flush(lobby);
  
  std::cout << "Info: room " << room.name << " starts" << std::endl;
}

void EnetLobby::process_event(ENetEvent& event) {
  switch (event.type) {
    case ENET_EVENT_TYPE_CONNECT: {
      clients_key[event.peer] = generate_key();
  
      {
        Packet<int> data = {.header=Header::Key, .value = clients_key[event.peer]};
        ENetPacket *packet_res = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
  
        enet_peer_send(event.peer, 0, packet_res);
      }
  
      {
        Packet<uint> data = {.header=Header::ClientId, .value = id_counter};
        subscribe(&data, clients_key[event.peer], sizeof(data));
        ENetPacket *packet_res = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(event.peer, 0, packet_res);
      }
      id_counter += 1;
      
      send_rooms_list(event.peer);
    }
      break;
    case ENET_EVENT_TYPE_RECEIVE: {
      subscribe(event.packet->data, clients_key[event.peer], event.packet->dataLength);
      Header head = *reinterpret_cast<Header *>(event.packet->data);
      if (head == Header::RoomList) {
        send_rooms_list(event.peer);
      }
      else if (head == Header::ConnectRoom) {
        uint id = reinterpret_cast<Packet<uint>*>(event.packet->data)->value;
        if (id < rooms.size() && rooms[id].clients.size() < rooms[id].max_clients_count) {
          rooms[id].clients[event.peer] = wait_room.clients[event.peer];
          wait_room.clients.erase(event.peer);
  
          {
            Packet<char> data = {.header=Header::ResultSuccess};
            subscribe(&data, clients_key[event.peer], sizeof(data));
            ENetPacket *packet_res = enet_packet_create(&data,
                                                        sizeof(data),
                                                        ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet_res);
          }
  
          for(auto& [client_peer, info] : rooms[id].clients) {
            if (event.peer != client_peer) {
              Packet<ClientInfo> data = {.header = Header::ClientAbout, .value = rooms[id].clients[event.peer]};
              subscribe(&data, clients_key[client_peer], sizeof(data));
              ENetPacket* packet = enet_packet_create (&data,
                                                       sizeof(data),
                                                       ENET_PACKET_FLAG_RELIABLE);
              enet_peer_send(client_peer, 0, packet);
      
              Packet<ClientInfo> data_to_new = {.header = Header::ClientAbout, .value = info};
              subscribe(&data_to_new, clients_key[event.peer], sizeof(data_to_new));
              ENetPacket* packet_to_new = enet_packet_create (&data_to_new,
                                                              sizeof(data_to_new),
                                                              ENET_PACKET_FLAG_RELIABLE);
              enet_peer_send(event.peer, 0, packet_to_new);
            }
          }
          enet_host_flush(lobby);
          
          if (rooms[id].clients.size() >= start_level && !rooms[id].game_start) {
            start_room(id);
          }
          else if (rooms[id].game_start) {
            std::array<char, 32> server_data{};
            std::copy_n(rooms[id].server.c_str(), rooms[id].server.length(), server_data.data());
            Packet<std::array<char, 32>> data = {.header = Header::ServerInfo, .value=server_data};
            subscribe(&data, clients_key[event.peer], sizeof(data));
            ENetPacket *packet = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
            enet_host_flush(lobby);
          }
        }
        else {
          Packet<char> data = {.header=Header::ResultFailed};
          subscribe(&data, clients_key[event.peer], sizeof(data));
          ENetPacket *packet_res = enet_packet_create(&data,
                                                      sizeof(data),
                                                      ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet_res);
        }
      }
      else if (head == Header::DisconnectRoom) {
        uint id = reinterpret_cast<Packet<uint>*>(event.packet->data)->value;;
        if (id < rooms.size() && rooms[id].clients.contains(event.peer)) {
          wait_room.clients[event.peer] = rooms[id].clients[event.peer];
          rooms[id].clients.erase(event.peer);
          
          for (auto& [client_peer, info] : rooms[id].clients) {
            Packet<ClientInfo> data = {.header=Header::DiscClientAbout, .value = wait_room.clients[event.peer]};
            subscribe(&data, clients_key[client_peer], sizeof(data));
            ENetPacket *packet_res = enet_packet_create(&data,
                                                        sizeof(data),
                                                        ENET_PACKET_FLAG_RELIABLE);
  
            enet_peer_send(client_peer, 0, packet_res);
          }
  
          {
            Packet<char> data = {.header=Header::ResultSuccess};
            subscribe(&data, clients_key[event.peer], sizeof(data));
            ENetPacket *packet_res = enet_packet_create(&data,
                                                        sizeof(data),
                                                        ENET_PACKET_FLAG_RELIABLE);
    
            enet_peer_send(event.peer, 0, packet_res);
          }
        }
        else {
          Packet<char> data = {.header=Header::ResultFailed};
          subscribe(&data, clients_key[event.peer], sizeof(data));
          ENetPacket *packet_res = enet_packet_create(&data,
                                                      sizeof(data),
                                                      ENET_PACKET_FLAG_RELIABLE);
          
          enet_peer_send(event.peer, 0, packet_res);
        }
      }
      else if (head == Header::NewRoomInfo) {
        RoomInfo info = reinterpret_cast<Packet<RoomInfo>*>(event.packet->data)->value;
        rooms.emplace_back(info.name, info.max_clients_count, info.start_radius, info.velocity, info.adaptive_vel);
        Packet<char> data = {.header=Header::ResultSuccess};
        subscribe(&data, clients_key[event.peer], sizeof(data));
        ENetPacket *packet_res = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(event.peer, 0, packet_res);
        std::cout << "Info: New room. " << std::string(info.name.data()) << ": " << rooms.size() - 1 << ". " << info.clients_count << "/" << info.max_clients_count
                  << ". ";
        if(info.game_start) {
          std::cout << "Running. ";
        }
        else {
          std::cout << "Waiting. ";
        }
        std::cout << "Settings: " << info.start_radius << ", " << info.velocity << "." << std::endl;
      }
      else if (head == Header::ClientAbout) {
        ClientInfo new_client = reinterpret_cast<Packet<ClientInfo>*>(event.packet->data)->value;
        wait_room.clients[event.peer] = new_client;
        
        std::cout << "Info: New client: name " << std::string(new_client.name.data()) << std::endl;
      }
      
      enet_packet_destroy(event.packet);
    }
      break;
    
    case ENET_EVENT_TYPE_DISCONNECT:
    {
      for (auto& room : rooms) {
        room.clients.erase(event.peer);
      }
      wait_room.clients.erase(event.peer);
      
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
  
  for (auto& room : rooms) {
    if (room.clients.empty()) {
      room.game_start = false;
      room.server = "localhost:";
    }
  }
}

void EnetLobby::Run() {
  ENetEvent event;
  for (auto& agent:agents) {
    ENetAddress address;
    
    enet_address_set_host (&address, agent.name.c_str());
    address.port = agent.port;
  
    agent.peer = enet_host_connect (lobby, & address, 2, 0);
    if (agent.peer == nullptr)
    {
      std::cout << "Error: No available peers for ENet connection" << std::endl;
    }
    
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (lobby, & event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
      std::cout << "Info: Connected to agent" << std::endl;
    }
    else
    {
      enet_peer_reset(agent.peer);
      std::cout << "Error: Connection to agent failed" << std::endl;
    }
  }
  
  while(working_flag) {
    while (enet_host_service(lobby, &event, 1000) > 0) {
      auto is_current = [&event](Agent& agent){ return agent.peer == event.peer; };
      auto result = std::find_if(agents.begin(), agents.end(), is_current);
      if (event.type == ENET_EVENT_TYPE_RECEIVE && result != agents.end()) {
        Header head = *reinterpret_cast<Header*>(event.packet->data);
        if (head == ServerPort) {
          uint port = reinterpret_cast<Packet<uint>*>(event.packet->data)->value;
          uint room_id = result->rooms_wait.front();
          result->rooms_wait.pop();
          
          if (!rooms[room_id].game_start) {
            rooms[room_id].server.append(std::to_string(port));
            rooms[room_id].game_start = true;
  
            std::array<char, 32> server_data{};
            std::copy_n(rooms[room_id].server.c_str(), rooms[room_id].server.length(), server_data.data());
            for (auto&[client_peer, info]: rooms[room_id].clients) {
              Packet<std::array<char, 32>> data = {.header = Header::ServerInfo, .value=server_data};
              subscribe(&data, clients_key[event.peer], sizeof(data));
              ENetPacket *packet = enet_packet_create(&data,
                                                      sizeof(data),
                                                      ENET_PACKET_FLAG_RELIABLE);
              enet_peer_send(client_peer, 0, packet);
            }
          }
          enet_packet_destroy(event.packet);
        }
      }
      else
        process_event(event);
    }
  }
}