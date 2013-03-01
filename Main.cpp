
#include <iostream>
#include <string.h>

#include "TinyTcl.h"

bool run = true;

tcl::ReturnCode exitProc(tcl::Context * ctx, tcl::ArgumentVector const& args, void * data)
{
  run = false;
  return tcl::RET_OK;
}

int main(int argc, char * argv[])
{
  std::cout << "Tiny Tcl" << std::endl;

  tcl::Context ctx;
  ctx.registerProc("exit", exitProc);

  std::string completeLine = "";

  if (argc > 1)
    for (int i = 1; i < argc; ++i)
      if (strcmp(argv[i], "-d") == 0)
        ctx.debug = true;

  while (run)
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
      if (ctx.evaluate(completeLine) == tcl::RET_ERROR)
        std::cout << "Error: " << ctx.error << std::endl;
      else if (!ctx.current().result.empty())
        std::cout << ctx.current().result << std::endl;
      completeLine = "";
    }
    else
    {
      completeLine += "\n";
    }
  }

  return 0;
}