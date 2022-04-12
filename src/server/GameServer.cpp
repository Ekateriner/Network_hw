#include "GameServer.h"
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>
#include <random>
#include <cstring>

GameServer::GameServer(uint32_t port) {
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

GameServer::~GameServer() {
  enet_host_destroy(server);
  enet_deinitialize();
}

void GameServer::create_server(uint32_t port) {
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

void GameServer::spawn_entities(uint32_t count) {
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<float> rand_x(0.0, weight);
  std::uniform_real_distribution<float> rand_y(0.0, height);
  std::uniform_real_distribution<float> rand_r(10.0, 20.0);
  //entities.emplace_back();
  for (int i = 1; i <= count; i++) {
    entities.push_back(Entity{.entity_id = static_cast<uint32_t>(i), .user_id = -1, .pos = std::make_pair(rand_x(gen), rand_y(gen)), .radius = rand_r(gen), .target = 0});
  }
}

void GameServer::spawn_user(int id) {
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<float> rand_x(0.0, weight);
  std::uniform_real_distribution<float> rand_y(0.0, height);
  //entities.emplace_back();
  entities.push_back(Entity{.entity_id = static_cast<uint32_t>(entities.size()) + 1, .user_id = id,
                            .pos = std::make_pair(rand_x(gen), rand_y(gen)), .radius = 20.0, .target = 0});
}

void GameServer::update_AI() {
  for (auto& entity1 : entities) {
    if (entity1.user_id != -1)
      continue;
    
    float eat_dist = height * height + weight * weight;
    float eat_id = entity1.entity_id;
    
    float predator_dist = height * height + weight * weight;
    float predator_id = entity1.entity_id;
    
    for (const auto& entity2 : entities) {
      if (entity1.entity_id != entity2.entity_id && entity1.radius > entity2.radius && dist2(entity1.pos, entity2.pos) < eat_dist) {
        eat_dist = dist2(entity1.pos, entity2.pos);
        eat_id = entity2.entity_id;
      }
      
      if (entity1.entity_id != entity2.entity_id && entity1.radius < entity2.radius && dist2(entity1.pos, entity2.pos) < predator_dist) {
        predator_dist = dist2(entity1.pos, entity2.pos);
        predator_id = entity2.entity_id;
      }
    }
    
    if (eat_dist < predator_dist) {
      entity1.target = eat_id;
    }
    else {
      entity1.target = -predator_id;
    }
  }
  
  for (auto& entity : entities) {
    if (entity.user_id != -1)
      continue;
    
    if (entity.target > 0) {
      std::pair<float, float> dir = (entities[entity.target - 1].pos - entity.pos) / dist(entities[entity.target - 1].pos, entity.pos);
      entity.pos = entity.pos + dir * dt;
    }
    else if (entity.target < 0) {
      std::pair<float, float> dir = (entities[-entity.target - 1].pos - entity.pos) / dist(entities[-entity.target - 1].pos, entity.pos);
      entity.pos = entity.pos - dir * dt;
    }
  }
}

void GameServer::check_cond() {
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<float> rand_x(0.0, weight);
  std::uniform_real_distribution<float> rand_y(0.0, height);
  
  for (auto& entity1 : entities) {
    for (auto& entity2 : entities) {
      if (entity1.entity_id != entity2.entity_id && entity1.radius > entity2.radius &&
          dist2(entity1.pos, entity2.pos) < (entity1.radius - entity2.radius) * (entity1.radius - entity2.radius)) {
        // 1 eats 2
        entity1.radius += entity2.radius / 2;
        entity2.radius /= 2;
        entity2.pos = std::make_pair(rand_x(gen), rand_y(gen));
      }
    }
  }
}

void GameServer::wall_check() {
  for (auto& entity : entities) {
    if (entity.pos.first - entity.radius < 0) {
      entity.pos.first = entity.radius;
    }
    if (entity.pos.first + entity.radius > weight) {
      entity.pos.first = weight - entity.radius;
    }
  
    if (entity.pos.second - entity.radius < 0) {
      entity.pos.second = entity.radius;
    }
    if (entity.pos.second + entity.radius > height) {
      entity.pos.second = height - entity.radius;
    }
  }
}

void GameServer::add_new_client(ENetPeer* peer, const std::string& name) {
  std::array<char, 64> temp{};
  std::sprintf(temp.data(), "%x:%u", peer->address.host, peer->address.port);
  std::string client_addr = std::string(temp.data());
  spawn_user(id_counter);
  clients_entities[peer] = entities.size();
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
  
  {
    std::array<int, 3> info = {clients[peer].id, weight, height};
    ENetPacket* packet = enet_packet_create (info.data(),
                                             sizeof(int) * 3,
                                             ENET_PACKET_FLAG_RELIABLE);
    enet_packet_resize (packet, sizeof(int) * 3 + 1);
    std::strcpy(reinterpret_cast<char*>(packet->data) + sizeof(int) * 3, "s");
    enet_peer_send(peer, 0, packet);
    enet_host_flush(server);
  }
  
  for(auto [client_peer, info] : clients) {
    if (peer != client_peer) {
      ENetPacket* packet = enet_packet_create (&clients[peer],
                                               sizeof(clients[peer]),
                                               ENET_PACKET_FLAG_RELIABLE);
      enet_packet_resize (packet, sizeof(clients[peer]) + 1);
      std::strcpy(reinterpret_cast<char*>(packet->data) + sizeof(clients[peer]), "o");
      enet_peer_send(client_peer, 0, packet);
  
      ENetPacket* packet_to_new = enet_packet_create (&clients[client_peer],
                                                      sizeof(clients[client_peer]),
                                                      ENET_PACKET_FLAG_RELIABLE);
  
      enet_packet_resize (packet_to_new, sizeof(clients[client_peer]) + 1);
      std::strcpy(reinterpret_cast<char*>(packet_to_new->data) + sizeof(clients[client_peer]), "o");
      enet_peer_send(peer, 0, packet_to_new);
    }
  }
  enet_host_flush(server);
}

void GameServer::resend_message(ENetPeer* peer, const std::string& message, size_t read_len) {
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

void GameServer::Run() {
  ENetEvent event;
  spawn_entities(10);
  while(working_flag) {
    while (enet_host_service(server, &event, 10) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
          if (event.channelID == 0) {
            std::string message = std::string(reinterpret_cast<char *>(event.packet->data),
                                              reinterpret_cast<char *>(event.packet->data) + event.packet->dataLength);
            if (clients.find(event.peer) == clients.end())
              add_new_client(event.peer, message);
            else {
              resend_message(event.peer, message, event.packet->dataLength);
            }
          }
          else {
            // get info
            auto target = reinterpret_cast<std::pair<float, float>*>(event.packet->data);
            auto entity = &entities[clients_entities[event.peer] - 1];
            std::pair<float, float> dir = (*target - entity->pos) / dist(*target, entity->pos);
            entity->pos = entity->pos + dir * dt * 10;
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
        {
          std::array<char, 64> temp{};
          std::sprintf(temp.data(), "%x:%u", event.peer->address.host, event.peer->address.port);
          std::string client_addr = std::string(temp.data());
          entities[clients_entities[event.peer] - 1].user_id = -1;
          clients.erase(event.peer);
          clients_entities.erase(event.peer);
          std::cout << "Info: " << client_addr << " has disconnected" << std::endl;
          event.peer -> data = NULL;
        }
          break;
        
        case ENET_EVENT_TYPE_NONE:
          break;
      }
    }
    if (!clients.empty()) {
      update_AI();
      check_cond();
      wall_check();
      
      for(auto [client_peer, info] : clients) {
        
        ENetPacket* packet = enet_packet_create (entities.data(),
                                                 sizeof(Entity) * entities.size(),
                                                 ENET_PACKET_FLAG_RELIABLE);
        enet_packet_resize (packet, sizeof(Entity) * entities.size() + 1);
        std::strcpy(reinterpret_cast<char*>(packet->data) + sizeof(Entity) * entities.size(), "e");
        enet_peer_send(client_peer, 0, packet);
      }
      enet_host_flush(server);
      //std::cout << "Info: broadcasting" << std::endl;
    }
  }
}