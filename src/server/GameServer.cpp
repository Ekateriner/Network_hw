#include "GameServer.h"
#include <csignal>
#include <iostream>
#include <random>
#include <cstring>
#include <chrono>
#include <bitset>
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
  for (int i = 0; i < count; i++) {
    entities[current_ts].push_back(Entity{.user_id = -1,
                                          .pos = std::make_pair(rand_x(gen), rand_y(gen)), .radius = rand_r(gen)});
  }
}

uint32_t GameServer::spawn_user(int id) {
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<float> rand_x(0.0, weight);
  std::uniform_real_distribution<float> rand_y(0.0, height);
  auto _id = static_cast<uint32_t>(entities[current_ts].size());
  entities[current_ts].push_back(Entity{.user_id = id,
                                        .pos = std::make_pair(rand_x(gen), rand_y(gen)), .radius = radius});
  return _id;
}

void GameServer::update_users() {
  for(auto& [peer, target] : last_target) {
    auto entity = &entities[current_ts][clients_entities[peer]];
    if (dist2(target, entity->pos) < 0.0001) {
      continue;
    }
    std::pair<float, float> dir = (target - entity->pos) / dist(target, entity->pos);
    entity->pos = entity->pos + dir * dt * velocity;
  }
}

void GameServer::update_AI() {
  std::vector<int> targets(entities[current_ts].size(), 0);
  for (size_t _id1 = 0; _id1 < entities[current_ts].size(); _id1++) {
    Entity& entity1 = entities[current_ts][_id1];
    
    if (entity1.user_id != -1)
      continue;
    
    float eat_dist = height * height + weight * weight;
    uint eat_id = _id1;
    
    float predator_dist = height * height + weight * weight;
    uint predator_id = _id1;
  
    for (size_t _id2 = 0; _id2 < entities[current_ts].size(); _id2++) {
      Entity& entity2 = entities[current_ts][_id2];
      if (_id1 != _id2 && entity1.radius > entity2.radius && dist2(entity1.pos, entity2.pos) < eat_dist) {
        eat_dist = dist2(entity1.pos, entity2.pos);
        eat_id = _id2;
      }
      
      if (_id1 != _id2 && entity1.radius < entity2.radius && dist2(entity1.pos, entity2.pos) < predator_dist) {
        predator_dist = dist2(entity1.pos, entity2.pos);
        predator_id = _id2;
      }
    }
    
    if (eat_dist < predator_dist) {
      targets[_id1] = int(eat_id + 1);
    }
    else {
      targets[_id1] = -int(predator_id + 1);
    }
  }
  
  for (size_t _id = 0; _id < entities[current_ts].size(); _id++) {
    Entity& entity = entities[current_ts][_id];
    if (entity.user_id != -1)
      continue;
    
    if (targets[_id] > 0) {
      std::pair<float, float> dir = (entities[current_ts][targets[_id]-1].pos - entity.pos) / dist(entities[current_ts][targets[_id]-1].pos, entity.pos);
      entity.pos = entity.pos + dir * dt;
    }
    else if (targets[_id] < 0) {
      std::pair<float, float> dir = (entities[current_ts][-targets[_id]-1].pos - entity.pos) / dist(entities[current_ts][-targets[_id]-1].pos, entity.pos);
      entity.pos = entity.pos - dir * dt;
    }
  }
}

void GameServer::check_cond() {
  std::random_device rd;  // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<float> rand_x(0.0, weight);
  std::uniform_real_distribution<float> rand_y(0.0, height);
  
  for (size_t _id1 = 0; _id1 < entities[current_ts].size(); _id1++) {
    for (size_t _id2 = 0; _id2 < entities[current_ts].size(); _id2++) {
      Entity& entity1 = entities[current_ts][_id1];
      Entity& entity2 = entities[current_ts][_id2];
      if (_id1 != _id2 && entity1.radius > entity2.radius &&
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
  for (auto& entity : entities[current_ts]) {
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
  uint32_t _id = spawn_user(info.id);
  clients_entities[peer] = _id;
  clients[peer] = info;
  last_target[peer] = entities[current_ts][_id].pos;
  
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
  entities[current_ts] = std::vector<Entity>();
  ENetEvent event;
  spawn_entities(10);
  while(working_flag) {
    while (enet_host_service(server, &event, 100) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          clients_key[event.peer] = 0;//generate_key();
          {
            Packet<int> data = {.header=Header::Key, .value = clients_key[event.peer]};
            ENetPacket *packet = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
          }
          //enet_host_flush(server);
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
            last_target[event.peer] = reinterpret_cast<Packet<std::pair<float, float>>*>(event.packet->data)->value;
          }
          else if (head == Header::ClientAnswer) {
            last_ts[event.peer] = reinterpret_cast<Packet<uint32_t>*>(event.packet->data)->value;
            auto min_ts = std::min_element(last_ts.begin(), last_ts.end(), [](auto& el1, auto& el2) { return el1.second < el2.second; } )->second;
            std::erase_if(entities, [&min_ts] (const auto& item) {
              auto const& [key, value] = item;
              return key < min_ts;
            });
          }
          enet_packet_destroy(event.packet);
        }
          break;
    
        case ENET_EVENT_TYPE_DISCONNECT:
        {
          std::array<char, 64> temp{};
          std::sprintf(temp.data(), "%x:%u", event.peer->address.host, event.peer->address.port);
          std::string client_addr = std::string(temp.data());
          entities[current_ts][clients_entities[event.peer]].user_id = -1;
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
    std::chrono::duration<double> elapsed_seconds = std::chrono::steady_clock::now() - start;
    if (!clients.empty()) {
      update_users();
      update_AI();
      check_cond();
      wall_check();
      
      for(auto& [client_peer, info] : clients) {
        BitVector data;
        std::bitset<sizeof(Header)*8> head(Header::Entities);
        for (int i = 0; i < sizeof(Header)*8; i++)
          data.push_back(head[i]);
        std::bitset<32> epoch(current_ts);
        for (int i = 0; i < 32; i++)
          data.push_back(epoch[i]);
        int ind = 0;
        for (auto& entity : entities[current_ts]) {
          if (!last_ts.contains(client_peer) || ind >= entities[last_ts[client_peer]].size()) {
            // its new entity
            std::bitset<32> value;
            value = std::bitset<32>(*reinterpret_cast<unsigned long *>(&entity.user_id));
            data.push_back(true);
            for (int i = 0; i < 32; i++)
              data.push_back(value[i]);
  
            value = std::bitset<32>(*reinterpret_cast<unsigned long *>(&entity.pos.first));
            data.push_back(true);
            for (int i = 0; i < 32; i++)
              data.push_back(value[i]);
  
            value = std::bitset<32>(*reinterpret_cast<unsigned long *>(&entity.pos.second));
            data.push_back(true);
            for (int i = 0; i < 32; i++)
              data.push_back(value[i]);
  
            value = std::bitset<32>(*reinterpret_cast<unsigned long *>(&entity.radius));
            data.push_back(true);
            for (int i = 0; i < 32; i++)
              data.push_back(value[i]);
          }
          else {
            Entity prev_entity = entities[last_ts[client_peer]][ind];
            if (prev_entity.user_id != entity.user_id) {
              std::bitset<32>value (*reinterpret_cast<unsigned long *>(&entity.user_id));
              data.push_back(true);
              for (int i = 0; i < 32; i++)
                data.push_back(value[i]);
            }
            else {
              data.push_back(false);
            }
            if (prev_entity.pos.first != entity.pos.first) {
              std::bitset<32>value (*reinterpret_cast<unsigned long *>(&entity.pos.first));
              data.push_back(true);
              for (int i = 0; i < 32; i++)
                data.push_back(value[i]);
            }
            else {
              data.push_back(false);
            }
            if (prev_entity.pos.second != entity.pos.second) {
              std::bitset<32>value (*reinterpret_cast<unsigned long *>(&entity.pos.second));
              data.push_back(true);
              for (int i = 0; i < 32; i++)
                data.push_back(value[i]);
            }
            else {
              data.push_back(false);
            }
            if (prev_entity.radius != entity.radius) {
              std::bitset<32>value (*reinterpret_cast<unsigned long *>(&entity.radius));
              data.push_back(true);
              for (int i = 0; i < 32; i++)
                data.push_back(value[i]);
            }
            else {
              data.push_back(false);
            }
          }
          ind += 1;
        }
        subscribe(data.data(), clients_key[client_peer], data.size());
        ENetPacket* packet = enet_packet_create (data.data(),
                                                 data.size(),
                                                 ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(client_peer, 1, packet);
      }
      enet_host_flush(server);
      
      current_ts += 1;
      entities[current_ts] = entities[current_ts - 1];
      //std::cout << "Info: broadcasting" << std::endl;
    }
    else if (elapsed_seconds.count() > 3000) {int
      working_flag = 0;
      break;
    }
  }
}