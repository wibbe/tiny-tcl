
#include <iostream>

#include "TinyTcl.h"

int main(int argc, char * arhv[])
{
  std::cout << "Tiny Tcl" << std::endl;

  tcl::Context ctx;

  std::string completeLine = "";

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
      if (!ctx.evaluate(completeLine, true))
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