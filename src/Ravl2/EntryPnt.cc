
#include <spdlog/spdlog.h>
#include "Ravl2/EntryPnt.hh"

namespace Ravl2
{
  namespace
  {
    FuncMainCallManagerT &mainFuncPtr()
    {
      static FuncMainCallManagerT gMainCallManager = nullptr;
      return gMainCallManager;
    }
  }

  FuncMainCallManagerT getMainCallManager()
  {
    return mainFuncPtr();
  }

  bool setMainCall(FuncMainCallManagerT entryPoint)
  {
    SPDLOG_INFO("Ravl2::setMainCall: setting custom main call manager {} ",static_cast<bool>(entryPoint));
    mainFuncPtr() = entryPoint;
    return true;
  }

  int ravlMain(int argc, char* argv[],FuncMainCallT func) {
    if(!mainFuncPtr()) {
      SPDLOG_INFO("No main func defined. ");
      return func(argc,argv);
    }
    SPDLOG_INFO("Passing control through...");
    return mainFuncPtr()(argc,argv,func);
  }
}
