#pragma once

#include <functional>

namespace Ravl2
{
  using FuncMainCallT = std::function<int (int argc, char** argv)>;
  using FuncMainCallManagerT = std::function<int (int argc, char** argv,FuncMainCallT userMain)>;

  //! @brief Access the current Main call manager.
  //! This is useful if you want to chain the main call manager.
  FuncMainCallManagerT getMainCall();

  //! @brief Register an alternate call to start the main program.
  //! This is used on MacOSX where for many libraries expect the main
  //! thread to be running in an event loop. It should be registered
  //! in the constructor of a static variable to ensure its setup
  //! before main() is entered.
  bool setMainCall(FuncMainCallManagerT entryPoint);

  //! @brief Call through function.
  //! This is intended to be called via the RAVL2_MAIN macro.
  int ravlMain(int argc, char* argv[], FuncMainCallT appMain);
}

#define RAVL2_MAIN \
ravl2AppMain(int, char**); \
int main(int argc, char** argv) { \
return Ravl2::ravlMain(argc,argv,ravl2AppMain); \
} \
int ravl2AppMain
