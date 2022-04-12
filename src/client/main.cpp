#include "EnetClient.h"
#include <iostream>

int main () {
  std::string name;
  std::cout << "Print name: ";
  std::cin  >> name;
  EnetClient client(name);
  client.Run();
  return 0;
}
