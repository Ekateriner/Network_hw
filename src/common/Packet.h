//
// Created by ekaterina on 22.05.2022.
//
#pragma once
#ifndef MIPT_NETWORK_PACKET_H
#define MIPT_NETWORK_PACKET_H

enum Header {
  CreateServer, ServerPort, // agent
  RoomList, ClientId, ClientAbout, NewRoomInfo, DiscClientAbout,
  ServerInfo, ConnectRoom, DisconnectRoom, ResultSuccess, ResultFailed, // lobby
  ChatMessage, MousePosition, // client
  GameInfo, Entities,  // server
  Key
};

template <typename T>
struct Packet{
  Header header;
  T value;
};

void subscribe(void* data, int key, int len) {
  for(auto [fragment, i] = std::tuple{reinterpret_cast<int*>(data), 0}; i < len; fragment += 1, i += sizeof(int)) {
    if (len - i < sizeof(int)) {
      *fragment = *fragment ^ (key & (1 << (sizeof(int) - len + i)));
    }
    else {
      *fragment = *fragment ^ key;
    }
  }
}

#endif //MIPT_NETWORK_PACKET_H
