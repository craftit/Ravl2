//
// Created by charles galambos on 21/09/2025.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Types.hh"

namespace Ravl2
{
  namespace
  {
    struct Base
    {
      virtual ~Base() = default;
      int i = 0;
    };

    struct Derived
      : public Base
    {
      int j = 0;
    };

  }

  TEST_CASE("typename")
  {
    std::string str;
    CHECK(typeName<std::string>() == "std::string");
    CHECK(typeName(str) == "std::string");
    CHECK(typeName(typeid(str)) == "std::string");
    registerTypeName(typeid(Base),"Test::Base");
    registerTypeName(typeid(Derived),"Test::Derived");
    Derived obj;
    Base objBase;
    Base *objPtr = &obj;
    CHECK(typeName(*objPtr) == "Test::Derived");
    CHECK(typeName<Base>() == "Test::Base");
    CHECK(typeName(obj) == "Test::Derived");
    CHECK(typeName(objBase) == "Test::Base");
    objPtr = &objBase;
    CHECK(typeName(*objPtr) == "Test::Base");
  }

}