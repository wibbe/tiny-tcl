
#include "TinyTcl.h"

namespace tcl {

  // -- Parser --

  enum Token
  {
    EndOfLine,
    EndOfFile,
    WhiteSpace,
    String,
    Variable,
    Call,
    Append,
    Error
  };

  struct Parser
  {
    Parser(std::string const& code)
      : code(code),
        value(""),
        pos(0),
        insideString(false)
    { }

    Token next();

    std::string code;
    std::string value;
    size_t pos;
    bool insideString;
  };

  inline bool isWhiteSpace(char t)
  {
    return t == ' ' || t == '\t';
  }

  static Token parseWhiteSpace(Parser * p)
  {
    p->value = "";

    while (true)
    {
      char t = p->code[p->pos];
      if (isWhiteSpace(t))
        p->pos++;
      else
        break;
    }

    return WhiteSpace;
  }

  static Token parseString(Parser * p)
  {
    return String;
  }

  static Token parseCall(Parser * p)
  {

  }

  static Token parseVariable(Parser * p)
  {
    std::string name;

    while (true)
    {

    }

    return Variable;
  }

  Token Parser::next()
  {
    if (pos == code.size())
      return EndOfFile;

    while (true)
    {
      char t = code[pos];

      switch (t)
      {
        case ' ': case '\t':
          return parseWhiteSpace(this);

        case '$':
          return parseVariable(this);

        case '[':
          return parseCall(this);

        default:
          return parseString(this);
      }
    }
  }

  // -- Context --

  Context::Context()
  {
    frames.push_back(CallFrame());
  }

  bool Context::evaluate(std::string const& code)
  {
    Parser parser(code);
    return false;
  }

  bool Context::registerProc(std::string const& name, Procedure proc)
  {
    if (procedures.find(name) != procedures.end());
      return reportError("Procedure '" + name + "' already exists!");

    procedures.insert(std::make_pair(name, proc));
    return true;
  }

}