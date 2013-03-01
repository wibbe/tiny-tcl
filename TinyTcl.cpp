
#include "TinyTcl.h"

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <stdio.h>

namespace tcl {

  extern double calculateExpr(Context * ctx, std::string const& _str);

  // -- Utils --

  void split(std::string const& input, std::string const& delims, std::vector<std::string> & result)
  {
    std::string::size_type lastPos = input.find_first_not_of(delims, 0);
    std::string::size_type pos = input.find_first_of(delims, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
      result.push_back(input.substr(lastPos, pos - lastPos));

      lastPos = input.find_first_not_of(delims, pos);
      pos = input.find_first_of(delims, lastPos);
    }
  }

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

  static const char * tokenAsReadable[] = {
    "EndOfLine",
    "EndOfFile",
    "Separator",
    "String",
    "Variable",
    "Escaped",
    "Command",
    "Append",
    "Error"
  };

  struct Parser
  {
    Parser(std::string const& code)
      : code(code),
        value(""),
        token(EndOfLine),
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

      p->value += p->current;
      p->inc();
    }

    return true;
  }

  static bool parseComment(Parser * p)
  {
    while (!p->eof() && p->current != '\n')
      p->inc();
    return true;
  }

  static bool parseCommand(Parser * p)
  {
    int outerLevel = 1;
    int innerLevel = 0;

    p->inc();
    p->value = "";

    while (true)
    {
      if (p->eof())
        break;
      else if (p->current == '[' && innerLevel == 0)
      {
        outerLevel++;
      }
      else if (p->current == ']' && innerLevel == 0)
      {
        if (--outerLevel == 0)
          break;
      }
      else if (p->current == '\\')
      {
        p->inc();
      }
      else if (p->current == '{')
      {
        innerLevel++;
      }
      else if (p->current == '}')
      {
        if (innerLevel > 0)
          innerLevel--;
      }

      p->value += p->current;
      p->inc();
    }

    if (p->current == ']')
      p->inc();

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

        case ' ': case '\t': case '\n': case ';':
          if (!p->insideString)
          {
            p->token = Escaped;
            return true;
          }
          break;

        case '"':
          if (p->insideString)
          {
            p->inc();
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

  bool Parser::next()
  {
    value = "";
    if (pos == code.size())
    {
      token = EndOfFile;
      return true;
    }

    while (true)
    {
      switch (current)
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
          parseComment(this);
          continue;

        default:
          return parseString(this);
      }
    }
  }

  // -- Build in functions --

  static ReturnCode builtInPuts(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() > 1)
      for (size_t i = 1, len = args.size(); i < len; ++i)
        std::cout << args[i] << (((i - 1) == len) ? "" : " ");

    std::cout << std::endl;

    return RET_OK;
  }

  static ReturnCode builtInSet(Context * ctx, ArgumentVector const& args, void * data)
  {
    const size_t len = args.size();

    if (len == 2)
    {
      return ctx->current().get(args[1], ctx->current().result) ? RET_OK : RET_ERROR;
    }
    else if (len == 3)
    {
      ctx->current().result = args[2];
      ctx->current().set(args[1], args[2]);
      return RET_OK;
    }
    else
      return ctx->arityError(args[0]);
  }

  static ReturnCode builtInExpr(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() == 1)
      return ctx->arityError(args[0]);

    std::string str;
    for (size_t i = 1; i < args.size(); ++i)
      str += args[i];

    ctx->reportError("");
    double result = calculateExpr(ctx, str);
    char buf[128];
    snprintf(buf, 128, "%f", result);
    ctx->current().result = std::string(buf);
    return ctx->error.empty() ? RET_OK : RET_ERROR;
  }

  static ReturnCode builtInIf(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 3 && args.size() != 5)
      return ctx->arityError(args[0]);

    double result = calculateExpr(ctx, args[1]);

    if (result > 0.0)
      return ctx->evaluate(args[2]);
    else if (args.size() == 5)
      return ctx->evaluate(args[4]);

    return RET_OK;
  }

  struct ProcData
  {
    std::vector<std::string> arguments;
    std::string body;
  };

  static ReturnCode builtInProcExec(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (!data)
      return ctx->reportError("Runtime error in '" + args[0] + "'");

    ProcData * procData = static_cast<ProcData *>(data);

    if ((args.size() - 1) != procData->arguments.size())
      return ctx->reportError("Procedure '" + args[0] + "' called with wrong number of arguments");

    ctx->frames.push_back(CallFrame());

    // Setup arguments
    for (size_t i = 0, len = procData->arguments.size(); i < len; ++i)
      ctx->current().set(procData->arguments[i], args[i + 1]);

    ReturnCode retCode = ctx->evaluate(procData->body);
    std::string result = ctx->current().result;
    ctx->frames.pop_back();
    ctx->current().result = result;

    return retCode;
  }

  static ReturnCode builtInProc(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 4)
      return ctx->arityError(args[0]);

    ProcData * procData = new ProcData;
    procData->body = args[3];
    split(args[2], " \t", procData->arguments);

    return ctx->registerProc(args[1], builtInProcExec, procData) ? RET_OK : RET_ERROR;
  }

  static ReturnCode builtInReturn(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 2)
      return ctx->arityError(args[0]);

    ctx->current().result = args[1];
    return RET_RETURN;
  }

  static ReturnCode builtInError(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 2)
      return ctx->arityError(args[0]);
    return ctx->reportError(args[1]);
  }

  static ReturnCode builtInEval(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 2)
      return ctx->arityError(args[0]);

    std::string str;
    for (size_t i = 1; i < args.size(); ++i)
      str += args[i] + " ";

    return ctx->evaluate(str);
  }

  static ReturnCode builtInWhile(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 3)
      return ctx->arityError(args[0]);

    const std::string check = "expr " + args[1];

    while (true)
    {
      ReturnCode retCode = ctx->evaluate(check);
      if (retCode != RET_OK)
        return retCode;

      if (std::atof(ctx->current().result.c_str()) > 0.0)
      {
        retCode = ctx->evaluate(args[2]);
        if (retCode == RET_OK || retCode == RET_CONTINUE)
          continue;
        else if (retCode == RET_BREAK)
          return RET_OK;
        else
          return retCode;
      }
      else
      {
        return RET_OK;
      }
    }
  }

  static ReturnCode buildInRetCode(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args[0] == "break")
      return RET_BREAK;
    return RET_CONTINUE;
  }

  static ReturnCode builtInIncr(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 2 && args.size() != 3)
      return ctx->arityError(args[0]);

    std::string var;
    if (!ctx->current().get(args[1], var))
      return ctx->reportError("Could not find variable '" + args[1] + "'");

    int inc = 1;
    int value = std::atoi(var.c_str());

    if (args.size() == 3)
      inc = std::atoi(args[2].c_str());

    char buf[32];
    snprintf(buf, 32, "%d", value + inc);
    ctx->current().set(args[1], std::string(buf));
    ctx->current().result = std::string(buf);
    return RET_OK;
  }

  // -- Context --

  Context::Context()
    : debug(false)
  {
    frames.push_back(CallFrame());
    registerProc("puts", &builtInPuts);
    registerProc("set", &builtInSet);
    registerProc("if", &builtInIf);
    registerProc("expr", &builtInExpr);
    registerProc("proc", &builtInProc);
    registerProc("return", &builtInReturn);
    registerProc("error", &builtInError);
    registerProc("eval", &builtInEval);
    registerProc("while", &builtInWhile);
    registerProc("break", &buildInRetCode);
    registerProc("continue", &buildInRetCode);
    registerProc("incr", &builtInIncr);
  }

  ReturnCode Context::reportError(std::string const& _error)
  {
    error = _error;
    return RET_ERROR;
  }

  ReturnCode Context::arityError(std::string const& command)
  {
    return reportError("Wrong number of arguments to procedure '" + command + "'");
  }

  ReturnCode Context::evaluate(std::string const& code)
  {
    Parser parser(code);

    current().result = "";
    std::vector<std::string> args;

    while (true)
    {
      Token previousToken = parser.token;
      if (!parser.next())
        return RET_ERROR;

      if (debug)
        std::cout << "Token: " << tokenAsReadable[parser.token] << " = '" << parser.value << "'" << std::endl;

      std::string value = parser.value;
      if (parser.token == Variable)
      {
        std::string var;
        if (current().get(value, var))
          current().result = value = var;
        else
          return reportError("Could not locate variable '" + value + "'");
      }
      else if (parser.token == Command)
      {
        if (!evaluate(parser.value))
          return RET_ERROR;

        value = current().result;
      }
      else if (parser.token == Separator)
      {
        previousToken = parser.token;
        continue;
      }

      if (parser.token == EndOfLine || parser.token == EndOfFile)
      {
        if (debug)
        {
          std::cout << "Evaluating: ";

          for (size_t i = 0; i < args.size(); ++i)
            std::cout << args[i] << ",";
          std::cout << std::endl;
        }

        if (args.size() >= 1)
        {
          ProcedureMap::iterator it = procedures.find(args[0]);
          if (it == procedures.end())
            return reportError("Could not find procedure '" + args[0] + "'");

          Procedure & proc = it->second;

          ReturnCode retCode = proc.callback(this, args, proc.data);
          if (retCode != RET_OK)
            return retCode;
        }

        args.clear();
      }

      if (!value.empty())
      {
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

      if (parser.token == EndOfFile)
        break;
    }

    return RET_OK;
  }

  bool Context::registerProc(std::string const& name, ProcedureCallback proc, void * data)
  {
    if (procedures.find(name) != procedures.end())
      return reportError("Procedure '" + name + "' already exists!");

    procedures.insert(std::make_pair(name, Procedure(proc, data)));
    return true;
  }

}