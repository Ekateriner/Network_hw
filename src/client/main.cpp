#include "Client.h"
#include <iostream>

int main () {
  std::string name;
  std::cout << "Print name: ";
  std::cin  >> name;
  Client client(name);
  client.Run();
  return 0;
}
