#include "GameClient.h"
#include <csignal>
#include <cstdlib>
#include <future>
#include <thread>
#include <cstring>
#include <sstream>
#include "../common/Packet.h"

using namespace std::chrono_literals;

GameClient::GameClient(std::string name, std::string lobby_name, uint32_t port) :
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

GameClient::~GameClient() {
  enet_peer_disconnect (lobby_peer, 0);
//  while (enet_host_service (client, & event, 3000) > 0)
//  {
//    switch (event.type)
//    {
//      case ENET_EVENT_TYPE_RECEIVE:
//        enet_packet_destroy(event.packet);
//        break;
//      case ENET_EVENT_TYPE_DISCONNECT:
//        std::cout << "Info: Disconnected" << std::endl;
//        return;
//    }
//  }
  enet_peer_reset (lobby_peer);
  
  enet_host_destroy(client);
  enet_deinitialize();
}

void GameClient::clean_game() {
  al_destroy_font(a_font);
  al_destroy_display(a_display);
  //al_destroy_timer(timer);
  al_destroy_event_queue(a_events);
  
  ENetEvent event;
  enet_peer_disconnect (server_peer, 0);
//  while (enet_host_service (client, & event, 3000) > 0)
//  {
//    switch (event.type)
//    {
//      case ENET_EVENT_TYPE_RECEIVE:
//        enet_packet_destroy(
//        packet);
//        break;
//      case ENET_EVENT_TYPE_DISCONNECT:
//        return;
//    }
//  }
  enet_peer_reset (server_peer);
  
};

void GameClient::init_allegro() {
  if(!al_init())
  {
    std::cout << "Error: Cannot initialize Allegro" << std::endl;
    return;
  }
  
//  if(!al_install_keyboard())
//  {
//    std::cout << "Error: Cannot install keyboard" << std::endl;
//    return;
//  }
  if(!al_install_mouse())
  {
    std::cout << "Error: Cannot install mouse" << std::endl;
    return;
  }
  
  a_timer = al_create_timer(1.0 / 100.0);
  if(!a_timer)
  {
    std::cout << "Error: Cannot initialize Allegro timer" << std::endl;
    return;
  }
  
  a_events = al_create_event_queue();
  if(!a_events)
  {
    std::cout << "Error: Cannot initialize Allegro queue" << std::endl;
    return;
  }
  
  a_display = al_create_display(weight, height);
  if(!a_display)
  {
    std::cout << "Error: Cannot initialize Allegro display" << std::endl;
    return;
  }
  
  a_font = al_create_builtin_font();
  if(!a_font)
  {
    std::cout << "Error: Cannot initialize Allegro font" << std::endl;
    return;
  }
  al_init_primitives_addon();
  al_register_event_source(a_events, al_get_mouse_event_source());
  //al_register_event_source(a_events, al_get_keyboard_event_source());
  al_register_event_source(a_events, al_get_display_event_source(a_display));
  al_register_event_source(a_events, al_get_timer_event_source(a_timer));
  
  al_start_timer(a_timer);
}

void GameClient::create_peer() {
  client = enet_host_create(NULL /* create a client host */,
                            2 /* only allow 1 outgoing connection */,
                            2 /* allow up to 2 channels to be used, 0 and 1 */,
                            0 /* assume any amount of incoming bandwidth */,
                            0 /* assume any amount of outgoing bandwidth */);
  if (client == nullptr)
  {
    std::cout << "Error: Cannot create client" << std::endl;
  }
}

void GameClient::send_message(const std::string& message) {
  std::array<char, 1024> server_data{};
  std::copy_n(message.c_str(), message.length(), server_data.data());
  
  Packet<std::array<char, 1024>> data = {.header = Header::ChatMessage, .value=server_data};
  //subscribe(&data, server_key, sizeof(data));
  ENetPacket* packet = enet_packet_create (&data,
                                           sizeof(data),
                                           ENET_PACKET_FLAG_RELIABLE);
  
  enet_peer_send(server_peer, 0, packet);
}

void GameClient::send_info(std::pair<float, float> mouse_pos) {
  Packet<std::pair<float, float>> data = {.header = Header::MousePosition, .value=mouse_pos};
  //subscribe(&data, server_key, sizeof(data));
  ENetPacket* packet = enet_packet_create (&data,
                                           sizeof(data),
                                           ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);

  enet_peer_send(server_peer, 1, packet);
}

void GameClient::draw(const std::vector<Entity> &entities) {
  if (!redraw)
    return;
  al_clear_to_color(al_map_rgb(0, 0, 0));
  for (auto entity : entities) {
    if (entity.user_id == -1) {
      al_draw_circle(entity.pos.first, entity.pos.second, entity.radius, al_map_rgb(0, 0, 255), 1);
      al_draw_text(a_font, al_map_rgb(0, 0, 255), entity.pos.first-entity.radius / 2, entity.pos.second, 0, std::to_string(entity.entity_id).c_str());
    }
    else if (entity.user_id == id) {
      al_draw_circle(entity.pos.first, entity.pos.second, entity.radius, al_map_rgb(0, 255, 0), 2);
      al_draw_text(a_font, al_map_rgb(0, 255, 0), entity.pos.first-entity.radius / 2, entity.pos.second, 0, name.c_str());
    }
    else {
      al_draw_circle(entity.pos.first, entity.pos.second, entity.radius, al_map_rgb(255, 0, 0), 1);
      al_draw_text(a_font, al_map_rgb(255, 0, 0), entity.pos.first-entity.radius / 2, entity.pos.second, 0, others[entity.user_id].c_str());
    }
  }
  al_flip_display();
  redraw = false;
}

void GameClient::process_event(ENetEvent& event) {
  switch (event.type) {
    case ENET_EVENT_TYPE_CONNECT:
      std::cout << "Error: Invalid client operation" << std::endl;
      break;
    
    case ENET_EVENT_TYPE_RECEIVE: {
      if (event.peer == lobby_peer) {
        subscribe(event.packet->data, lobby_key, event.packet->dataLength);
      }
      else if (event.peer == server_peer) {
        //std::cout << "Server packet" << std::endl;
        //subscribe(event.packet->data, server_key, event.packet->dataLength);
      }
      
      Header head = *reinterpret_cast<Header*>(event.packet->data);
      
      if (head == Header::ClientAbout) {
        auto other = reinterpret_cast<Packet<ClientInfo> *>(event.packet->data)->value;
        others[other.id] = std::string(other.name.data());
      }
      else if (head == Header::DiscClientAbout) {
        auto other = reinterpret_cast<Packet<ClientInfo> *>(event.packet->data)->value;
        others.erase(other.id);
      }
      else if (head == Header::ServerInfo) {
        std::string server_str(reinterpret_cast<Packet<std::array<char, 32>> *>(event.packet->data)->value.data());
        //std::cout << "Connect to server " << server_str << std::endl;
        uint del = server_str.find(':');
  
        ENetAddress address;
  
        enet_address_set_host(&address, server_str.substr(0, del).c_str());
        address.port = stoul(server_str.substr(del + 1));
  
        server_peer = enet_host_connect(client, &address, 2, 0);
        if (server_peer == nullptr) {
          std::cout << "Error: No available peers for ENet connection" << std::endl;
          break;
        }
  
        ENetEvent inner_event;
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        if (enet_host_service(client, &inner_event, 5000) > 0 &&
                inner_event.type == ENET_EVENT_TYPE_CONNECT) {
          std::cout << "Info: Connected to server" << std::endl;
        } else {
          enet_peer_reset(server_peer);
          std::cout << "Error: Connection to server failed" << std::endl;
  
          Packet<uint> data = {.header=Header::DisconnectRoom, .value=uint(room_id)};
          subscribe(&data, lobby_key, sizeof(data));
          ENetPacket* packet = enet_packet_create (&data,
                                                   sizeof(data),
                                                   ENET_PACKET_FLAG_RELIABLE);
  
          enet_peer_send(lobby_peer, 0, packet);
  
          ENetEvent wait_event;
          /* Wait up to 5 seconds for the connection attempt to succeed. */
          while (enet_host_service (client, & wait_event, 5000) > 0 &&
                  wait_event.type == ENET_EVENT_TYPE_RECEIVE) {
            subscribe(wait_event.packet->data, lobby_key, wait_event.packet->dataLength);
            Header head = *reinterpret_cast<Header*>(wait_event.packet->data);
            if (head == Header::ResultSuccess) {
              std::cout << "Info: Disconnection success" << std::endl;
              room_id = -1;
              others.clear();
              if (in_game) {
                clean_game();
                in_game = false;
              }
              enet_packet_destroy(wait_event.packet);
              break;
            }
            else if (head == Header::ResultFailed) {
              std::cout << "Info: Disconnection failed" << std::endl;
              enet_packet_destroy(wait_event.packet);
              break;
            }
            else {
              process_event(wait_event);
            }
          }
          break;
        }
        
        if (enet_host_service (client, & inner_event, 5000) > 0 &&
                inner_event.type == ENET_EVENT_TYPE_RECEIVE) {
          Header head = *reinterpret_cast<Header*>(inner_event.packet->data);
          if (head != Header::Key) {
            std::cout << "Error: Server didn't send key" << std::endl;
          }
          else {
            //std::cout << "Info: got key" << std::endl;
            server_key = reinterpret_cast<Packet<int>*>(event.packet->data)->value;
          }
          enet_packet_destroy(inner_event.packet);
        }
        
        std::array<char, 64> name_array{};
        std::sprintf(name_array.data(), "%s", name.c_str());
        ClientInfo info = {.id = id, .name=name_array};
  
        Packet<ClientInfo> data = {.header = Header::ClientAbout, .value = info};
        //subscribe(&data, server_key, sizeof(data));
        ENetPacket *packet_res = enet_packet_create(&data,
                                                    sizeof(data),
                                                    ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(server_peer, 0, packet_res);
        //std::cout << "Info: send self clientInfo" << std::endl;
      }
      else if (head == Header::GameInfo) {
        auto info = reinterpret_cast<Packet<std::array<int, 2>>*>(event.packet->data)->value;
        weight = info[0];
        height = info[1];
        init_allegro();
        
        in_game = true;
      }
      else if (head == Header::Entities) {
        // get entities
        std::vector<Entity> to_draw;
        std::array<Entity, 4048> data = reinterpret_cast<Packet<std::array<Entity, 4048>> *> (event.packet->data)->value;
        std::copy_if(data.begin(), data.end(), std::back_inserter(to_draw), [](Entity& e) { return e.radius > 0.0001 || e.user_id != -1; });
        draw(to_draw);
      }
      else if (head == Header::ChatMessage) {
        std::string message(reinterpret_cast<Packet<std::array<char, 1024>> *> (event.packet->data)->value.data());
        std::cout << "Message: " << message << std::endl;
      }
      else {
        std::cout << "Warn: Unknown packet" << std::endl;
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

void GameClient::Run() {
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
        event.type == ENET_EVENT_TYPE_CONNECT) {
      std::cout << "Info: Connected to lobby" << std::endl;
  
      if (enet_host_service (client, & event, 5000) > 0 &&
          event.type == ENET_EVENT_TYPE_RECEIVE) {
        Header head = *reinterpret_cast<Header*>(event.packet->data);
        if (head != Header::Key) {
          std::cout << "Error: Lobby didn't send key" << std::endl;
        }
        else {
          lobby_key = reinterpret_cast<Packet<int>*>(event.packet->data)->value;
        }
        enet_packet_destroy(event.packet);
      }
      
      if (enet_host_service (client, & event, 5000) > 0 &&
          event.type == ENET_EVENT_TYPE_RECEIVE) {
        subscribe(event.packet->data, lobby_key, event.packet->dataLength);
        Header head = *reinterpret_cast<Header*>(event.packet->data);
        if (head != Header::ClientId) {
          std::cout << "Error: Lobby didn't send id" << std::endl;
        }
        else {
          id = int(reinterpret_cast<Packet<uint>*>(event.packet->data)->value);
        }
        enet_packet_destroy(event.packet);
      }
  
      std::array<char, 64> name_array{};
      std::sprintf(name_array.data(), "%s", name.c_str());
      ClientInfo info = {.id = id, .name=name_array};
  
      Packet<ClientInfo> data = {.header = Header::ClientAbout, .value = info};
      subscribe(&data, lobby_key, sizeof(data));
      ENetPacket *packet_res = enet_packet_create(&data,
                                                  sizeof(data),
                                                  ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(lobby_peer, 0, packet_res);
  
      while (enet_host_service (client, & event, 5000) > 0 &&
             event.type == ENET_EVENT_TYPE_RECEIVE) {
        subscribe(event.packet->data, lobby_key, event.packet->dataLength);
        Header head = *reinterpret_cast<Header *>(event.packet->data);
        if (head == Header::RoomList) {
          std::array<RoomInfo, 256> rooms = reinterpret_cast<Packet<std::array<RoomInfo, 256>>*>(event.packet->data)->value;
          std::cout << "Room_name: id. now/max. Status. Settings: radius, velocity." << std::endl;
          for(size_t i = 0; i < rooms.size(); i++) {
            if (rooms[i].max_clients_count == 0)
              continue;
            std::cout << std::string(rooms[i].name.data()) << ": " << i << ". " << rooms[i].clients_count << "/" << rooms[i].max_clients_count
                      << ". ";
            if(rooms[i].game_start) {
              std::cout << "Running. ";
            }
            else {
              std::cout << "Waiting. ";
            }
            std::cout << "Settings: " << rooms[i].start_radius << ", " << rooms[i].velocity << "." << std::endl;
          }
          enet_packet_destroy(event.packet);
          break;
        }
        else {
          process_event(event);
        }
      }
    }
    else
    {
      enet_peer_reset(lobby_peer);
      std::cout << "Error: Connection to lobby failed" << std::endl;
    }
  }
  
  auto input = std::async(std::launch::async, console_input);
  
  while(working_flag != 0) {
    if (input.wait_for(0ms) == std::future_status::ready) {
      auto input_string = input.get();
      input = std::async(std::launch::async, console_input);
      if (creation_stage) {
        RoomInfo info{};
        std::string room_name;
        std::istringstream(input_string) >> room_name >> info.max_clients_count >> info.start_radius >> info.velocity >> info.adaptive_vel;
        std::copy_n(room_name.c_str(), room_name.length(), info.name.data());
        
        Packet<RoomInfo> data = {.header=Header::NewRoomInfo, .value=info};
        subscribe(&data, lobby_key, sizeof(data));
        ENetPacket* packet = enet_packet_create (&data,
                                                 sizeof(data),
                                                 ENET_PACKET_FLAG_RELIABLE);
  
        enet_peer_send(lobby_peer, 0, packet);
  
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        while (enet_host_service (client, & event, 5000) > 0 &&
               event.type == ENET_EVENT_TYPE_RECEIVE) {
          subscribe(event.packet->data, lobby_key, event.packet->dataLength);
          Header head = *reinterpret_cast<Header*>(event.packet->data);
          if (head == Header::ResultSuccess) {
            std::cout << "Info: Creation success" << std::endl;
            enet_packet_destroy(event.packet);
            break;
          }
          else if (head == Header::ResultFailed) {
            std::cout << "Info: Creation failed" << std::endl;
            enet_packet_destroy(event.packet);
            break;
          }
          else {
            process_event(event);
          }
        }
        creation_stage = false;
      }
      else if (input_string == "/rooms") {
        Packet<char> data = {.header=Header::RoomList};
        subscribe(&data, lobby_key, sizeof(data));
        ENetPacket* packet = enet_packet_create (&data,
                                                 sizeof(data),
                                                 ENET_PACKET_FLAG_RELIABLE);
  
        enet_peer_send(lobby_peer, 0, packet);
  
        while (enet_host_service (client, & event, 5000) > 0 &&
               event.type == ENET_EVENT_TYPE_RECEIVE) {
          subscribe(event.packet->data, lobby_key, event.packet->dataLength);
          Header head = *reinterpret_cast<Header *>(event.packet->data);
          if (head == Header::RoomList) {
            std::array<RoomInfo, 256> rooms = reinterpret_cast<Packet<std::array<RoomInfo, 256>>*>(event.packet->data)->value;
            std::cout << "Room_name: id. now/max. Status. Settings: radius, velocity." << std::endl;
            for(size_t i = 0; i < rooms.size(); i++) {
              if (rooms[i].max_clients_count == 0)
                continue;
              std::cout << std::string(rooms[i].name.data()) << ": " << i << ". " << rooms[i].clients_count << "/" << rooms[i].max_clients_count
              << ". ";
              if(rooms[i].game_start) {
                std::cout << "Running. ";
              }
              else {
                std::cout << "Waiting. ";
              }
              std::cout << "Settings: " << rooms[i].start_radius << ", " << rooms[i].velocity << "." << std::endl;
            }
            enet_packet_destroy(event.packet);
            break;
          }
          else {
            process_event(event);
          }
        }
      }
      else if (input_string == "/exit") {
        working_flag = 0;
        if (room_id != -1) {
          Packet<uint> data = {.header=Header::DisconnectRoom, .value=uint(room_id)};
          subscribe(&data, lobby_key, sizeof(data));
          ENetPacket *packet = enet_packet_create(&data,
                                                  sizeof(data),
                                                  ENET_PACKET_FLAG_RELIABLE);
  
          enet_peer_send(lobby_peer, 0, packet);
  
          /* Wait up to 5 seconds for the connection attempt to succeed. */
          while (enet_host_service(client, &event, 5000) > 0 &&
                 event.type == ENET_EVENT_TYPE_RECEIVE) {
            subscribe(event.packet->data, lobby_key, event.packet->dataLength);
            Header head = *reinterpret_cast<Header *>(event.packet->data);
            if (head == Header::ResultSuccess) {
              std::cout << "Info: Disconnection success" << std::endl;
              room_id = -1;
              others.clear();
              if (in_game) {
                clean_game();
                in_game = false;
              }
              enet_packet_destroy(event.packet);
              break;
            } else if (head == Header::ResultFailed) {
              std::cout << "Info: Disconnection failed" << std::endl;
              enet_packet_destroy(event.packet);
              break;
            } else {
              process_event(event);
            }
          }
        }
      }
      else if (input_string == "/other" && room_id != -1) {
        for (auto& client_info : others) {
          std::cout << "Id: " << client_info.first << " / Name: " << client_info.second << std::endl;
        }
      }
      else if(input_string.starts_with("/connect ") && room_id == -1) {
        Packet<uint> data = {.header=Header::ConnectRoom, .value=uint(std::stoi(input_string.substr(9)))};
        subscribe(&data, lobby_key, sizeof(data));
        ENetPacket* packet = enet_packet_create (&data,
                                                 sizeof(data),
                                                 ENET_PACKET_FLAG_RELIABLE);
  
        enet_peer_send(lobby_peer, 0, packet);
  
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        while (enet_host_service (client, & event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_RECEIVE) {
          subscribe(event.packet->data, lobby_key, event.packet->dataLength);
          Header head = *reinterpret_cast<Header*>(event.packet->data);
          if (head == Header::ResultSuccess) {
            std::cout << "Info: Connection success" << std::endl;
            room_id = stoi(input_string.substr(9));
            enet_packet_destroy(event.packet);
            break;
          }
          else if (head == Header::ResultFailed) {
            std::cout << "Info: Connection failed" << std::endl;
            enet_packet_destroy(event.packet);
            break;
          }
          else {
            process_event(event);
          }
        }
      }
      else if(input_string == "/disconnect" && room_id != -1) {
        Packet<uint> data = {.header=Header::DisconnectRoom, .value=uint(room_id)};
        subscribe(&data, lobby_key, sizeof(data));
        ENetPacket* packet = enet_packet_create (&data,
                                                 sizeof(data),
                                                 ENET_PACKET_FLAG_RELIABLE);
  
        enet_peer_send(lobby_peer, 0, packet);
  
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        while (enet_host_service (client, & event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_RECEIVE) {
          subscribe(event.packet->data, lobby_key, event.packet->dataLength);
          Header head = *reinterpret_cast<Header*>(event.packet->data);
          if (head == Header::ResultSuccess) {
            std::cout << "Info: Disconnection success" << std::endl;
            room_id = -1;
            others.clear();
            if (in_game) {
              clean_game();
              in_game = false;
            }
            enet_packet_destroy(event.packet);
            break;
          }
          else if (head == Header::ResultFailed) {
            std::cout << "Info: Disconnection failed" << std::endl;
            enet_packet_destroy(event.packet);
            break;
          }
          else {
            process_event(event);
          }
        }
      }
      else if (input_string == "/create") {
        std::cout << "Write room params: 'name' 'max players' 'start radius' 'velocity' 'adaptive mod(0 or 1)" << std::endl;
        creation_stage = true;
      }
      else if (in_game) {
        send_message(input_string);
      }
    }
    
    if (in_game) {
      ALLEGRO_EVENT a_event;
    
      while (!al_is_event_queue_empty(a_events)) {
        al_wait_for_event_timed(a_events, &a_event, 0);
        switch (a_event.type) {
          case ALLEGRO_EVENT_MOUSE_AXES:
            send_info(std::make_pair(a_event.mouse.x, a_event.mouse.y));
            break;
        
          case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
        }
      }
    }
    
    
    while ( enet_host_service(client, &event, 10) > 0) {
      process_event(event);
    }
  }
}