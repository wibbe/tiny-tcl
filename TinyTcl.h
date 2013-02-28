
#pragma onces

#include <string>
#include <map>
#include <vector>

namespace tcl {

  struct Context;
  struct CallFrame;

  typedef int (*Procedure)(Context * ctx, std::vector<std::string> const& args, void * data);

  typedef std::map<std::string, std::string> VariableMap;

  struct CallFrame
  {
    CallFrame()
      : result("")
    { }

    void set(std::string const& name, std::string const& value)
    {
      variables[name] = value;
    }

    std::string get(std::string const& name) const
    {
      VariableMap::const_iterator result = variables.find(name);
      return result == variables.end() ? "" : result->second;
    }

    VariableMap variables;
    std::string result;
  };

  typedef std::map<std::string, Procedure> ProcedureMap;
  typedef std::vector<CallFrame> CallFrameVector;

  struct Context
  {
    Context();

    bool evaluate(std::string const& code);
    bool registerProc(std::string const& name, Procedure proc);

    bool reportError(std::string const& _error)
    {
      error = _error;
      return false;
    }

    CallFrame & current() { return frames.back(); }

    ProcedureMap procedures;
    CallFrameVector frames;

    std::string error;
  };

}