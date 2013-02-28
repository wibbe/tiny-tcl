
#pragma onces

#include <string>
#include <map>
#include <vector>

namespace tcl {

  struct Context;
  struct CallFrame;

  typedef bool (*Procedure)(Context * ctx, std::vector<std::string> const& args, void * data);

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

    bool get(std::string const& name, std::string & value) const
    {
      VariableMap::const_iterator result = variables.find(name);
      if (result == variables.end())
        return false;
      value = result->second;
      return true;
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