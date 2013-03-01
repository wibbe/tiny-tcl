
#pragma onces

#include <string>
#include <map>
#include <vector>

namespace tcl {

  struct Context;
  struct CallFrame;

  enum ReturnCode
  {
    RET_ERROR,
    RET_OK,
    RET_RETURN,
    RET_BREAK,
    RET_CONTINUE
  };

  typedef std::vector<std::string> ArgumentVector;
  typedef ReturnCode (*ProcedureCallback)(Context * ctx, ArgumentVector const& args, void * data);

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

    std::string get(std::string const& name)
    {
      VariableMap::const_iterator result = variables.find(name);
      return result == variables.end() ? "" : result->second;
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

  struct Procedure
  {
    Procedure(ProcedureCallback callback, void * data)
      : callback(callback),
        data(data)
    { }

    ProcedureCallback callback;
    void * data;
  };

  typedef std::map<std::string, Procedure> ProcedureMap;
  typedef std::vector<CallFrame> CallFrameVector;

  struct Context
  {
    Context();

    ReturnCode evaluate(std::string const& code);
    bool registerProc(std::string const& name, ProcedureCallback proc, void * data = 0);

    ReturnCode arityError(std::string const& command);
    ReturnCode reportError(std::string const& _error);

    CallFrame & current() { return frames.back(); }

    ProcedureMap procedures;
    CallFrameVector frames;

    std::string error;
    bool debug;
  };

}