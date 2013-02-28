
#include "TinyTcl.h"

#include <iostream>
#include <cstdlib>
#include <cmath>

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

  static bool builtInPuts(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() > 1)
      for (size_t i = 1, len = args.size(); i < len; ++i)
        std::cout << args[i] << (((i - 1) == len) ? "" : " ");

    std::cout << std::endl;

    return true;
  }

  static bool builtInSet(Context * ctx, ArgumentVector const& args, void * data)
  {
    const size_t len = args.size();

    if (len == 2)
    {
      return ctx->current().get(args[1], ctx->current().result);
    }
    else if (len == 3)
    {
      ctx->current().result = args[2];
      ctx->current().set(args[1], args[2]);
      return true;
    }
    else
      return ctx->arityError(args[0]);
  }

  static std::vector<char *> split(char * str)
  {
    std::vector<char *> result;
    char * start = NULL;

    for (char * it = str; *it != '\0'; ++it)
    {
      switch (*it)
      {
        case ' ': case '\t':
          if (start)
          {
            *it = '\0';
            result.push_back(start);
            start = NULL;
          }
          continue;

        default:
          if (!start)
            start = it;
      }
    }

    if (start)
      result.push_back(start);

    return result;
  }

  enum Operation
  {
    OpNone,
    OpMul,
    OpDiv,
    OpRest,
    OpAdd,
    OpSub,
    OpLess,
    OpGreater,
    OpEqual,
    OpNotEqual,
    OpLogicAnd,
    OpLogicOr
  };

  static double calc(double a, double b, Operation op)
  {
    switch (op)
    {
      case OpNone:
        return 0.0;
      case OpMul:
        return a * b;
      case OpDiv:
        return a / b;
      case OpRest:
        return (double)((long)a % (long)b);
      case OpAdd:
        return a + b;
      case OpSub:
        return a - b;
      case OpLess:
        return a < b ? 1.0 : 0.0;
      case OpGreater:
        return a > b ? 1.0 : 0.0;
      case OpEqual:
        return std::abs(a - b) < 0.0000001 ? 1.0 : 0.0;
      case OpNotEqual:
        return std::abs(a - b) > 0.0000001 ? 1.0 : 0.0;
      case OpLogicAnd:
        return a > 0.0 && b > 0.0 ? 1.0 : 0.0;
      case OpLogicOr:
        return a > 0.0 || b > 0.0 ? 1.0 : 0.0;
    }
  }

  static double calculateExpr(Context * ctx, std::string const& _str)
  {
    char * str = strndup(_str.c_str(), _str.size());
    std::vector<char *> tokens = split(str);
    std::vector<double> values;
    std::vector<Operation> operand;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      char * token = tokens[i];

      if (token[0] >= '0' && token[0] <= '9')
      {
        values.push_back(std::atof(token));
      }
      else if (token[0] == '$' && ctx)
      {
        values.push_back(std::atof(ctx->current().get(token).c_str()));
      }
      else
      {
        Operation op = OpNone;

        switch (token[0])
        {
          case '*':
            op = OpMul;
            break;
          case '/':
            op = OpDiv;
            break;
          case '%':
            op = OpRest;
            break;
          case '+':
            op = OpAdd;
            break;
          case '-':
            op = OpSub;
            break;
          case '<':
            op = OpLess;
            break;
          case '>':
            op = OpGreater;
            break;
          case '=':
            if (token[1] == '=')
              op = OpEqual;
            break;
          case '!':
            if (token[1] == '=')
              op = OpNotEqual;
            break;
          case '&':
            if (token[1] == '&')
              op = OpLogicAnd;
            break;
          case '|':
            if (token[1] == '|')
              op = OpLogicOr;
            break;
        }

        if (op != OpNone)
        {
          if (operand.empty() || operand.back() > op)
            operand.push_back(op);
          else if (values.size() >= 2 && !operand.empty())
          {
            Operation opToApply = operand.back();
            operand.pop_back();
            operand.push_back(op);

            double b = values.back();
            values.pop_back();
            double a = values.back();
            values.pop_back();
            values.push_back(calc(a, b, opToApply));
          }
        }
      }
    }

    // Apply the last operands
    while (!operand.empty() && values.size() >= 2)
    {
      Operation op = operand.back();
      operand.pop_back();

      double b = values.back();
      values.pop_back();
      double a = values.back();
      values.pop_back();
      values.push_back(calc(a, b, op));
    }

    free(str);

    return values.empty() ? 0.0 : values.back();
  }

  static bool builtInExpr(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() == 1)
      return ctx->arityError(args[0]);

    std::string str;
    for (size_t i = 1; i < args.size(); ++i)
      str += args[i];

    double result = calculateExpr(ctx, str);
    char buf[128];
    snprintf(buf, 128, "%f", result);
    ctx->current().result = std::string(buf);
    return true;
  }

  static bool builtInIf(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 3 && args.size() != 5)
      return ctx->arityError(args[0]);

    if (!ctx->evaluate(args[1]))
      return false;

    if (std::atoi(ctx->current().result.c_str()) != 0)
      return ctx->evaluate(args[2]);
    else if (args.size() == 5)
      return ctx->evaluate(args[4]);

    return true;
  }

  static bool builtInMatch(Context * ctx, ArgumentVector const& args, void * data)
  {
    if (args.size() != 3)
      return ctx->arityError(args[0]);

    if (args[0] == "==")
      ctx->current().result = std::atoi(args[1].c_str()) == std::atoi(args[2].c_str()) ? "1" : "0";
    else if (args[0] == "!=")
      ctx->current().result = std::atoi(args[1].c_str()) != std::atoi(args[2].c_str()) ? "1" : "0";

    return true;
  }

  // -- Context --

  Context::Context()
  {
    frames.push_back(CallFrame());
    registerProc("puts", &builtInPuts);
    registerProc("set", &builtInSet);
    registerProc("if", &builtInIf);
    registerProc("==", &builtInMatch);
    registerProc("!=", &builtInMatch);
    registerProc("expr", &builtInExpr);
  }

  bool Context::arityError(std::string const& command)
  {
    return reportError("Wrong number of arguments to procedure '" + command + "'");
  }

  bool Context::evaluate(std::string const& code, bool debug)
  {
    Parser parser(code);

    current().result = "";
    std::vector<std::string> args;

    while (true)
    {
      Token previousToken = parser.token;
      if (!parser.next())
        return false;

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
        if (!evaluate(parser.value, debug))
          return false;

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

          bool success = proc.callback(this, args, proc.data);
          if (!success)
            return false;
        }

        args.clear();
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

      if (parser.token == EndOfFile)
        break;
    }

    return true;
  }

  bool Context::registerProc(std::string const& name, ProcedureCallback proc, void * data)
  {
    if (procedures.find(name) != procedures.end())
      return reportError("Procedure '" + name + "' already exists!");

    procedures.insert(std::make_pair(name, Procedure(proc, data)));
    return true;
  }

}