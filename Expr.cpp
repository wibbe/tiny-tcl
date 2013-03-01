
#include "TinyTcl.h"

#include <cstdlib>
#include <cmath>
#include <stack>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

namespace tcl {

  double evalMinus(double a, double b)
  {
    return -a;
  }

  double evalMul(double a, double b)
  {
    return a * b;
  }

  double evalDiv(double a, double b)
  {
    return a / b;
  }

  double evalAdd(double a, double b)
  {
    return a + b;
  }

  double evalSub(double a, double b)
  {
    return a - b;
  }

  double evalLess(double a, double b)
  {
    return a < b ? 1.0 : 0.0;
  }

  double evalGreater(double a, double b)
  {
    return a > b ? 1.0 : 0.0;
  }

  double evalEqual(double a, double b)
  {
    return std::abs(a - b) < 0.0000001 ? 1.0 : 0.0;
  }

  double evalNotEqual(double a, double b)
  {
    return std::abs(a - b) > 0.0000001 ? 1.0 : 0.0;
  }

  double evalLogicAnd(double a, double b)
  {
    return a > 0.0 && b > 0.0 ? 1.0 : 0.0;
  }

  double evalLogicOr(double a, double b)
  {
    return a > 0.0 || b > 0.0 ? 1.0 : 0.0;
  }

  enum
  {
    ASSOC_NONE,
    ASSOC_LEFT,
    ASSOC_RIGHT
  };

  struct Operand
  {
    const char * op;
    short len;
    short precedence;
    unsigned char assoc;
    bool unary;
    double (*eval)(double a, double b);
  };

  static Operand _operands[] = {
    {"~",  1, 11, ASSOC_RIGHT,  true, evalMinus},
    {"*",  1, 10,  ASSOC_LEFT, false, evalMul},
    {"/",  1, 10,  ASSOC_LEFT, false, evalDiv},
    {"+",  1,  9,  ASSOC_LEFT, false, evalAdd},
    {"-",  1,  9,  ASSOC_LEFT, false, evalSub},
    {"<",  1,  7,  ASSOC_LEFT, false, evalLess},
    {">",  1,  7,  ASSOC_LEFT, false, evalGreater},
    {"==", 2,  6,  ASSOC_LEFT, false, evalEqual},
    {"!=", 2,  6,  ASSOC_LEFT, false, evalNotEqual},
    {"&&", 2,  2,  ASSOC_LEFT, false, evalLogicAnd},
    {"||", 2,  1,  ASSOC_LEFT, false, evalLogicOr},
    {"(",  1,  0,  ASSOC_NONE, false, NULL},
    {")",  1,  0,  ASSOC_NONE, false, NULL}
  };

  inline Operand * getOperand(const char * op)
  {
    static const int len = sizeof(_operands) / sizeof(Operand);

    for (int i = 0; i < len; ++i)
    {
      if (strncmp(op, _operands[i].op, _operands[i].len) == 0)
        return &_operands[i];
    }

    return 0;
  }

  static bool applyOperand(Context * ctx, Operand * op, std::stack<double> & values, std::stack<Operand *> & operands)
  {
    double a, b;

    if (op->op[0] == '(')
    {
      operands.push(op);
      return true;
    }
    else if (op->op[0] == ')')
    {
      while (!operands.empty() && operands.top()->op[0] != '(')
      {
        Operand * top = operands.top();
        operands.pop();
        b = values.top();
        values.pop();

        if (top->unary)
        {
          values.push(top->eval(b, 0.0));
        }
        else
        {
          a = values.top();
          values.pop();

          values.push(top->eval(a, b));
        }
      }

      Operand * top = operands.empty() ? 0 : operands.top();
      operands.pop();

      if (!top || top->op[0] != '(')
      {
        ctx->reportError("Stack error, no matching \'(\'");
        return false;
      }

      return true;
    }

    if (op->assoc == ASSOC_RIGHT)
    {
      while (!operands.empty() && op->precedence < operands.top()->precedence)
      {
        Operand * top = operands.top();
        operands.pop();

        b = values.top();
        values.pop();

        if (top->unary)
        {
          values.push(top->eval(b, 0.0));
        }
        else
        {
          a = values.top();
          values.pop();

          values.push(top->eval(a, b));
        }
      }
    }
    else
    {
      while (!operands.empty() && op->precedence <= operands.top()->precedence)
      {
        Operand * top = operands.top();
        operands.pop();

        b = values.top();
        values.pop();

        if (top->unary)
        {
          values.push(top->eval(b, 0.0));
        }
        else
        {
          a = values.top();
          values.pop();

          values.push(top->eval(a, b));
        }
      }
    }

    operands.push(op);
    return true;
  }

  double calculateExpr(Context * ctx, std::string const& str)
  {
    //char * str = strdup(_str.c_str());
    std::stack<double> values;
    std::stack<Operand *> operands;

    Operand startOp = {"X", 0, ASSOC_NONE, 0, NULL};
    Operand * lastOp = &startOp;
    const char * digit = NULL;

    for (const char * it = str.c_str(); *it; ++it)
    {
      if (!digit)
      {
        if (isdigit(*it))
        {
          digit = it;
        }
        else if (Operand * op = getOperand(it))
        {
          if (lastOp && (lastOp == &startOp || lastOp->op[0] == ')'))
          {
            if (op->op[0] == '-')
            {
              op = getOperand("~");
            }
            else if (op->op[0] != '(')
            {
              ctx->reportError("Illegal use of operator (" + std::string(op->op) + ")");
              return 0.0;
            }
          }

          it += op->len - 1;

          if (!applyOperand(ctx, op, values, operands))
            return 0.0;

          lastOp = op;
        }
        else if (!isspace(*it))
        {
          ctx->reportError("Syntax error in expr '" + str + "'");
          return 0.0;
        }
      }
      else
      {
        if (isspace(*it))
        {
          values.push(std::atof(digit));
          digit = NULL;
          lastOp = NULL;
        }
        else if (Operand * op = getOperand(it))
        {
          values.push(std::atof(digit));

          it += op->len - 1;
          digit = NULL;

          if (!applyOperand(ctx, op, values, operands))
            return 0.0;

          lastOp = op;
        }
        else if (!isdigit(*it) && *it != '.')
        {
          ctx->reportError("Syntax error in expr '" + str + "'");
          return 0.0;
        }
      }
    }

    if (digit)
      values.push(std::atof(digit));

    while (!operands.empty())
    {
      Operand * op = operands.top();
      operands.pop();

      double b = values.top();
      values.pop();

      if (op->unary)
      {
        values.push(op->eval(b, 0.0));
      }
      else
      {
        double a = values.top();
        values.pop();
        values.push(op->eval(a, b));
      }
    }

    if (values.size() != 1)
    {
      ctx->reportError("Error while executing expr '" + str + "'");
      return 0.0;
    }

    return values.top();
  }

}
