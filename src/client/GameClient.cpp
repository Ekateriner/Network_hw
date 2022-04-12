#include "GameClient.h"
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <future>
#include <thread>

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
  ENetEvent event;
  enet_peer_disconnect (server_peer, 0);
//  while (enet_host_service (client, & event, 3000) > 0)
//  {
//    switch (event.type)
//    {
//      case ENET_EVENT_TYPE_RECEIVE:
//        enet_packet_destroy(event.packet);
//        break;
//      case ENET_EVENT_TYPE_DISCONNECT:
//        return;
//    }
//  }
  enet_peer_reset (server_peer);
  
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
  
  al_destroy_font(a_font);
  al_destroy_display(a_display);
  //al_destroy_timer(timer);
  al_destroy_event_queue(a_events);
  
  enet_host_destroy(client);
  enet_deinitialize();
}

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

void GameClient::send_message(const std::string& message, ENetPeer* peer) {
  ENetPacket* packet = enet_packet_create (message.c_str(),
                                           message.length(),
                                           ENET_PACKET_FLAG_RELIABLE);
  
  enet_peer_send(peer, 0, packet);
}

void GameClient::send_info(ENetPeer* peer, std::pair<float, float> mouse_pos) {
  ENetPacket* packet = enet_packet_create (&mouse_pos,
                                           sizeof(mouse_pos),
                                           ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);

  enet_peer_send(peer, 1, packet);
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
      else if (input_string == "/exit") {
        working_flag = 0;
      }
      else if (input_string == "/other" && in_game) {
        for (auto client_info : others) {
          std::cout << "Id: " << client_info.first << " / Name: " << client_info.second << std::endl;
        }
      }
      else if (in_game) {
        send_message(input_string, server_peer);
      }
    }
  
    if (id != -1) {
      ALLEGRO_EVENT a_event;
    
      while (!al_is_event_queue_empty(a_events)) {
        al_wait_for_event_timed(a_events, &a_event, 0);
        switch (a_event.type) {
          case ALLEGRO_EVENT_MOUSE_AXES:
            mouse_pos = std::make_pair(a_event.mouse.x, a_event.mouse.y);
            break;
        
          case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
        }
      }
    }
    
    
    while ( enet_host_service(client, &event, 10) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          std::cout << "Error: Invalid client operation" << std::endl;
          break;
    
        case ENET_EVENT_TYPE_RECEIVE: {
          std::string message = std::string(reinterpret_cast<char*>(event.packet->data),
                                            reinterpret_cast<char*>(event.packet->data) + event.packet->dataLength);
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
              enet_peer_reset(server_peer);
              std::cout << "Error: Connection to server failed" << std::endl;
            }
            in_game = true;
  
            send_message(name, server_peer);
        
          } else if (event.peer == server_peer && event.channelID == 0) {
            if (message[event.packet->dataLength - 1] == 's') {
              int* info = reinterpret_cast<int*>(event.packet->data);
              id = *info;
              weight = *(info + 1);
              height = *(info + 2);
              init_allegro();
            }
            else if (message[event.packet->dataLength - 1] == 'o') {
              auto other = reinterpret_cast<ClientInfo *>(event.packet->data);
              others[other->id] = std::string(other->name.data());
            }
            else if (message[event.packet->dataLength - 1] == 'e') {
              // get entities
              std::vector<Entity> to_draw(reinterpret_cast<Entity*>(event.packet->data),
                                          reinterpret_cast<Entity*>(event.packet->data) + (event.packet->dataLength - 1) / sizeof(Entity));
              draw(to_draw);
            }
          }
          else if (event.peer == server_peer && event.channelID == 1) {
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
    send_info(server_peer, mouse_pos);
  }
}