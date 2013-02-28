
#include <iostream>

#include "TinyTcl.h"

int main(int argc, char * arhv[])
{
  std::cout << "Tiny Tcl" << std::endl;

  tcl::Context ctx;

  while (true)
  {
    char line[1024];

    std::cout << "> ";
    std::cin.getline(line, 1024);

    if (ctx.evaluate(std::string(line), true))
    {

    }
    else
      std::cout << "Error: " << ctx.error << std::endl;
  }

  return 0;
}