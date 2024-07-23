//
// Created by charles galambos on 23/07/2024.
//

#include "Ravl2/Assert.hh"

int main(int nargs,char **args) {
  RavlAssertMsg(1 == 1, "This is a test message.");
  RavlAlwaysAssertMsg(1 == 0, "This test failed.");
  return 0;
}