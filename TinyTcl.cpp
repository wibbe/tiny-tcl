
#include "TinyTcl.h"

#include <iostream>

namespace tcl {

  // -- Parser --

  enum Token
  {
    EndOfLine,
    EndOfFile,
    Separator,
    String,
    Variable,
    Escaped,
    Command,
    Append,
    Error
  };

  struct Parser
  {
    Parser(std::string const& code)
      : code(code),
        value(""),
        current(code.empty() ? 0 : code[0]),
        pos(0),
        insideString(false)
    { }

    bool next();
    void inc() { if (pos < code.size()) current = code[++pos]; }
    bool eof() const { return len() <= 0; }
    size_t len() const { return code.size() - pos; }

    std::string code;
    std::string value;
    Token token;
    char current;
    size_t pos;
    bool insideString;
  };

  inline bool isSeparator(char t)
  {
    return t == ' ' || t == '\t' || t == '\n';
  }

  static bool parseSeparator(Parser * p)
  {
    p->value = "";

    while (true)
    {
      if (isSeparator(p->current))
        p->inc();
      else
        break;
    }
    p->token = Separator;
    return true;
  }

  static bool parseBracet(Parser * p)
  {
    int level = 1;
    p->inc();
    p->value = "";

    while (true)
    {
      if (p->len() >= 2 && p->current == '\\')
      {
        p->inc();
        p->value += p->current;
      }
      else if (p->len() == 0 || p->current == '}')
      {
        level--;

        if (level == 0 || p->len() == 0)
        {
          if (!p->eof())
            p->inc();

          p->token = String;
          return true;
        }
      }
      else if (p->current == '{')
      {
        level++;
      }

      p->inc();
      p->value += p->current;
    }

    return true;
  }

  static bool parseString(Parser * p)
  {
    const bool newWord = (p->token == Separator || p->token == EndOfLine || p->token == String);

    if (newWord && p->current == '{')
      return parseBracet(p);
    else if (newWord && p->current == '"')
    {
      p->insideString = true;
      p->inc();
    }

    p->value = "";

    while (true)
    {
      if (p->eof())
      {
        p->token = Escaped;
        return true;
      }

      switch (p->current)
      {
        case '\\':
          if (p->len() >= 2)
          {
            p->value += p->current;
            p->inc();
          }
          break;

        case '$': case '[':
          p->token = Escaped;
          return true;

        case '"':
          if (p->insideString)
          {
            p->token = Escaped;
            p->insideString = false;
            return true;
          }
          break;
      }

      p->value += p->current;
      p->inc();
    }

    return String;
  }

  static bool parseComment(Parser * p)
  {
    while (!p->eof() && p->current != '\n')
      p->inc();
    return true;
  }

  static bool parseCommand(Parser * p)
  {
    p->token = Command;
    return true;
  }

  static bool parseEndOfLine(Parser * p)
  {
    while (isSeparator(p->current) || p->current == ';')
      p->inc();

    p->token = EndOfLine;
    return true;
  }

  static bool parseVariable(Parser * p)
  {
    p->value = "";
    p->inc(); // eat $

    while (true)
    {
      if ((p->current >= 'a' && p->current <= 'z') || (p->current >= 'A' && p->current <= 'Z') || (p->current >= '0' && p->current <= '9'))
      {
        p->value += p->current;
        p->inc();
      }
      else
        break;
    }

    if (p->value.empty()) // This was just a single character string "$"
    {
      p->value = "$";
      p->token = String;
    }
    else
      p->token = Variable;

    return true;
  }

  bool Parser::next()
  {
    if (pos == code.size())
    {
      token = EndOfFile;
      return true;
    }

    while (true)
    {
      char t = code[pos];

      switch (t)
      {
        case ' ': case '\t': case '\r':
          if (insideString)
            return parseString(this);
          else
            return parseSeparator(this);

        case '\n': case ';':
          if (insideString)
            return parseString(this);
          else
            return parseEndOfLine(this);

        case '$':
          return parseVariable(this);

        case '[':
          return parseCommand(this);

        case '#':
          if (token == EndOfLine)
          {
            parseComment(this);
            continue;
          }

          return parseString(this);

        default:
          return parseString(this);
      }
    }
  }

  // -- Context --

  static bool buildInPuts(Context * ctx, std::vector<std::string> const& args, void * data);

  Context::Context()
  {
    frames.push_back(CallFrame());
    registerProc("puts", &buildInPuts);
  }

  bool Context::evaluate(std::string const& code)
  {
    Parser parser(code);

    current().result = "";
    std::vector<std::string> args;

    while (true)
    {
      Token previousToken = parser.token;
      if (!parser.next())
        return false;

      if (parser.token == EndOfFile)
        break;

      std::string value = parser.value;
      if (parser.token == Variable)
      {
        std::string var;
        if (current().get(value, var))
          value = var;
        else
          return reportError("Could not locate variable '" + value + "'");
      }
      else if (parser.token == Command)
      {
        if (!evaluate(parser.value))
          return false;

        value = current().result;
      }
      else if (parser.token == Separator)
      {
        previousToken = parser.token;
        continue;
      }

      if (previousToken == Separator || previousToken == EndOfLine)
      {
        args.push_back(value);
      }
      else
      {
        if (args.empty())
          args.push_back(value);
        else
          args.back() += value;
      }
    }

    return true;
  }

  bool Context::registerProc(std::string const& name, Procedure proc)
  {
    if (procedures.find(name) != procedures.end());
      return reportError("Procedure '" + name + "' already exists!");

    procedures.insert(std::make_pair(name, proc));
    return true;
  }

  // -- Build in functions --

  static bool buildInPuts(Context * ctx, std::vector<std::string> const& args, void * data)
  {
    if (args.size() > 1)
      for (size_t i = 1, len = args.size(); i < len; ++i)
        std::cout << args[i] << (((i - 1) == len) ? "" : " ");

    return true;
  }

}