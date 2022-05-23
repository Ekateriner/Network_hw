#include "GameServer.h"
#include <csignal>
#include <iostream>
#include <random>
#include <cstring>
#include <chrono>
#include "../common/Packet.h"

GameServer::GameServer(uint32_t port, float _radius, float _velocity):
  radius(_radius),
  velocity(_velocity)
{
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
  std::uniform_real_distribution<float> rand_r(radius / 2.0, radius);
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
                            .pos = std::make_pair(rand_x(gen), rand_y(gen)), .radius = radius, .target = 0});
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

void GameServer::add_new_client(ENetPeer* peer, const ClientInfo& info) {
  std::array<char, 64> temp{};
  std::sprintf(temp.data(), "%x:%u", peer->address.host, peer->address.port);
  std::string client_addr = std::string(temp.data());
  spawn_user(info.id);
  clients_entities[peer] = entities.size();
  clients[peer] = info;
  
  //generate kay
  
  std::array<int, 2> game_info = {weight, height};
  
  Packet<std::array<int, 2>> data = {.header = Header::GameInfo, .value = game_info};
  subscribe(&data, clients_key[peer], sizeof(data));
  ENetPacket* packet = enet_packet_create (&data,
                                           sizeof(data),
                                           ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
  enet_host_flush(server);
}

void GameServer::resend_message(ENetPeer* peer, const std::string& message) {
  std::array<char, 1024> text{};
  std::sprintf(text.data(), "From %s (id - %d): %s", clients[peer].name.data(), clients[peer].id, message.c_str());
  std::cout << "Info: " << std::string(text.data()) << std::endl;
  
  for(auto [client_peer, info] : clients) {
    if (peer != client_peer) {
      Packet<std::array<char, 1024>> data = {.header=Header::ChatMessage, .value = text};
      subscribe(&data, clients_key[client_peer], sizeof(data));
      ENetPacket * packet = enet_packet_create (&data,
                                                sizeof(data),
                                                ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
  
      enet_peer_send (client_peer, 1, packet);
    }
  }
  enet_host_flush(server);
}

void GameServer::Run() {
  auto start = std::chrono::steady_clock::now();
  ENetEvent event;
  spawn_entities(10);
  while(working_flag) {
    while (enet_host_service(server, &event, 10) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          clients_key[event.peer] = generate_key();
          Packet<int> data = {.header=Header::Key, .value = clients_key[event.peer]};
          ENetPacket *packet_res = enet_packet_create(&data,
                                                      sizeof(data),
                                                      ENET_PACKET_FLAG_RELIABLE);
  
          enet_peer_send(event.peer, 0, packet_res);
          break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
          subscribe(event.packet->data, clients_key[event.peer], event.packet->dataLength);
          Header head = *reinterpret_cast<Header*>(event.packet->data);
          if (head == Header::ClientAbout) {
            ClientInfo info = reinterpret_cast<Packet<ClientInfo>*>(event.packet->data)->value;
  
            if (clients.find(event.peer) == clients.end()) {
              add_new_client(event.peer, info);
            }
            else {
              clients[event.peer].name = info.name;
            }
          }
          else if (head == Header::ChatMessage) {
            std::string message(reinterpret_cast<Packet<std::array<char, 1024>>*>(event.packet->data)->value.data());
            resend_message(event.peer, message);
          }
          else if (head == Header::MousePosition) {
            // get info
            auto target = reinterpret_cast<Packet<std::pair<float, float>>*>(event.packet->data)->value;
            auto entity = &entities[clients_entities[event.peer] - 1];
            std::pair<float, float> dir = (target - entity->pos) / dist(target, entity->pos);
            entity->pos = entity->pos + dir * dt * velocity;
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
    std::chrono::duration<double> elapsed_seconds = std::chrono::steady_clock::now()-start;
    if (!clients.empty()) {
      update_AI();
      check_cond();
      wall_check();
      
      for(auto [client_peer, info] : clients) {
        std::array<Entity, 4048> entities_data{};
        std::copy_n(entities.data(), entities.size(), entities_data.data());
        Packet<std::array<Entity, 4048>> data = {.header = Header::Entities, .value = entities_data};
        subscribe(&data, clients_key[client_peer], sizeof(data));
        ENetPacket* packet = enet_packet_create (&data,
                                                 sizeof(data),
                                                 ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(client_peer, 1, packet);
      }
      enet_host_flush(server);
      //std::cout << "Info: broadcasting" << std::endl;
    }
    else if (elapsed_seconds.count() > 30) {
      working_flag = 0;
      break;
    }
  }
}