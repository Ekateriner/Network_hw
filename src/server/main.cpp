#include "GameServer.h"

int main(int argc, char *argv[]){
  GameServer server(std::stoul(argv[1]), std::stof(argv[2]), std::stof(argv[3]));
  server.Run();
  return 0;
}
