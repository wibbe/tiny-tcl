
#include <iostream>
#include <string.h>

#include "TinyTcl.h"

int main(int argc, char * argv[])
{
  std::cout << "Tiny Tcl" << std::endl;

  tcl::Context ctx;

  std::string completeLine = "";

  bool debug = false;
  if (argc > 1)
    for (int i = 1; i < argc; ++i)
      if (strcmp(argv[i], "-d") == 0)
        debug = true;

  while (true)
  {
    char line[1024];

    if (completeLine.empty())
      std::cout << "> ";
    else
      std::cout << "| ";

    std::cin.getline(line, 1024);

    completeLine += std::string(line);
    int braceCount = 0;
    for (int i = 0; i < completeLine.size(); ++i)
    {
      switch (completeLine[i])
      {
        case '{':
          braceCount++;
          break;
        case '[':
          braceCount++;
          break;
        case '}':
          braceCount--;
          break;
        case ']':
          braceCount--;
          break;
      }
    }

    if (braceCount == 0)
    {
      if (!ctx.evaluate(completeLine, debug))
        std::cout << "Error: " << ctx.error << std::endl;
      completeLine = "";
    }
    else
    {
      completeLine += "\n";
    }
  }

  return 0;
}