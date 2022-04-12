#include "GameClient.h"
#include <iostream>

int main () {
  std::string name;
  std::cout << "Print name: ";
  std::cin  >> name;
  GameClient client(name);
  client.Run();
  return 0;
}
