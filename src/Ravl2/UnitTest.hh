// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2009, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_UNITTEST_HEADER
#define RAVL_UNITTEST_HEADER 1
/////////////////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Core.Misc"
//! file="Ravl/Core/Base/UnitTest.hh"
//! lib=RavlCore
//! userlevel=Normal
//! author="Charles Galambos"

#include <string.h>
#include <cstdlib>
#include <cmath>

// Helper functions for unit testing RAVL code.
namespace Ravl2 {


  template<typename Data1T, typename Data2T>
  bool TestEquals(const Data1T &expected,const Data2T &actual, const char* file, int line) {
    if (!(expected == actual)) {
      std::cerr << file << ":" << line << " Expected value (" << expected << ") but instead got (" << actual << ")\n";
      return false;
    }
    return true;
  }

  bool TestEquals(const char* actual, const char* expected, const char* file, int line) {
    if (strcmp(expected, actual) != 0) {
      std::cerr << file << ":" << line << " Expected string \n" << expected << "\nbut instead got\n" << actual << std::endl;
      return false;
    }
    return true;
  }

  template<typename Data1T, typename Data2T>
    bool TestNotEquals(const Data2T &actual, const Data1T &expected,const char* file, int line) {
    if (expected == actual) {
      std::cerr << file << ":" << line << " Value should not equal (" << expected << ")\n";
      return false;
    }
    return true;
  }

  template<typename DataT>
  bool TestTrue(const DataT &value, const char* file, int line) {
    if (!value) {
      std::cerr << file << ":" << line << " Expression should be true\n";
      return false;
    }
    return true;
  }

  template<typename DataT>
  bool TestFalse(const DataT &value, const char* file, int line) {
    if (value) {
      std::cerr << file << ":" << line << " Expression should be false\n";
      return false;
    }
    return true;
  }

  template<typename Data1T, typename Data2T, typename Data3T>
    bool TestAlmostEquals(const Data2T &actual, const Data1T &expected, const Data3T &tolerance, const char* file, int line) {
    if (std::abs(expected - actual) > tolerance) {
      std::cerr << file << ":" << line << " Actual value (" << actual << ") not within range (" << expected << " +- " << tolerance << ")\n";
      return false;
    }
    return true;
  }

  template<typename Data1T, typename Data2T>
  bool TestGreaterThan(const Data1T &value, const Data2T &threshold, const char* file, int line) {
    if (!(value > threshold)) {
      std::cerr << file << ":" << line << " Expected value (" << value << ") to be greater than (" << threshold << ")\n";
      return false;
    }
    return true;
  }

  template<typename Data1T, typename Data2T>
  bool TestLessThan(const Data1T &value, const Data2T &threshold, const char* file, int line) {
    if (!(value < threshold)) {
      std::cerr << file << ":" << line << " Expected value (" << value << ") to be less than (" << threshold << ")\n";
      return false;
    }
    return true;
  }

}

#define RAVL_RUN_TEST(x) { \
  int ln; \
  std::cerr << "Testing '" #x "' " << std::flush; \
  if((ln = x) != 0) { \
    std::cerr << "\r " << #x " FAILED on line " << ln << std::endl; \
    return 1; \
  } else { \
    std::cerr << "\r " << #x " PASSED. \n" << std::flush; \
  } \
}

#define RAVL_TEST_EQUALS(x,y) { \
  if (!Ravl2::TestEquals((x), (y), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_NOT_EQUALS(x,y) { \
  if (!Ravl2::TestNotEquals((x), (y), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_TRUE(x) { \
  if (!Ravl2::TestTrue((x), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_FALSE(x) { \
  if (!Ravl2::TestFalse((x), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_ALMOST_EQUALS(x,y,t) { \
  if (!Ravl2::TestAlmostEquals((x), (y), (t), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_THROWS(expr, exc, expected) { \
  try { \
    expr; \
    std::cerr << __FILE__ << ":" << __LINE__ << " Expression " << #expr << " didn't throw exception " << #exc << std::endl; \
    return __LINE__; \
  } catch (const exc &ex) { \
    RAVL_TEST_EQUALS(expected, ex.Text()); \
  } catch (...) { \
    std::cerr << __FILE__ << ":" << __LINE__ << " Expression " << #expr << " threw unexpected exception " << std::endl; \
    throw; \
  } \
}

#define RAVL_TEST_GREATER_THAN(x,y) { \
  if (!Ravl2::TestGreaterThan((x), (y), __FILE__, __LINE__)) \
    return __LINE__; \
}

#define RAVL_TEST_LESS_THAN(x,y) { \
  if (!Ravl2::TestLessThan((x), (y), __FILE__, __LINE__)) \
    return __LINE__; \
}

#endif
