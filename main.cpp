#include "server.h"
#include <iostream>

int main()
{
  try
  {
    std::cout << "Start app on 8080" << std::endl;
    server s(8080);
    s.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}