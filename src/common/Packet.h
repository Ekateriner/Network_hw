//
// Created by ekaterina on 22.05.2022.
//

#ifndef MIPT_NETWORK_PACKET_H
#define MIPT_NETWORK_PACKET_H

enum Header {
  CreateServer, ServerPort, // agent
  RoomList, ClientId, ClientAbout, NewRoomInfo, DiscClientAbout,
  ServerInfo, ConnectRoom, DisconnectRoom, ResultSuccess, ResultFailed, // lobby
  ChatMessage, MousePosition, // client
  GameInfo, Entities  // server
};

template <typename T>
struct Packet{
  Header header;
  T value;
};

#endif //MIPT_NETWORK_PACKET_H
