//
// Created by charles galambos on 06/10/2023.
//

#include "checks.hh"
#include "Ravl2/CallbackArray.hh"

TEST_CASE("CallbackArray")
{
  SECTION("Simple")
  {
    Ravl2::CallbackArray<std::function<void()>> cbArray;
    EXPECT_TRUE(cbArray.empty());
    int count = 0;
    auto handle = cbArray.add([&count]() { count++; });
    EXPECT_FALSE(cbArray.empty());
    cbArray.call();
    EXPECT_EQ(count, 1);
    auto handle2 = cbArray.add([&count]() { count++; });
    EXPECT_FALSE(cbArray.empty());
    cbArray.call();
    EXPECT_EQ(count, 3);
    EXPECT_TRUE(handle.isActive());
    handle.remove();
    EXPECT_FALSE(handle.isActive());
    cbArray.call();
    EXPECT_EQ(count, 4);

    cbArray.clear();
    EXPECT_TRUE(cbArray.empty());
    cbArray.call();
    EXPECT_EQ(count, 4);
    handle2.remove();
    cbArray.call();
    EXPECT_EQ(count, 4);
  }
  SECTION("ReusedHandle")
  {
    Ravl2::CallbackArray<std::function<void()>> cbArray;
    EXPECT_TRUE(cbArray.empty());
    int count = 0;
    auto handle = cbArray.add([&count]() { count++; });
    EXPECT_FALSE(cbArray.empty());
    EXPECT_TRUE(handle.isActive());
    // Copy the handle, if either has removed the callback it will be removed, so the it will become stale.
    auto handle2 = handle;
    cbArray.call();
    EXPECT_EQ(count, 1);
    handle.remove();
    EXPECT_FALSE(handle.isActive());
    cbArray.call();
    EXPECT_EQ(count, 1);

    // Put a new handle, now handle2 may point to a reused callback slot.
    auto handle3 = cbArray.add([&count]() { count++; });
    EXPECT_TRUE(handle2.isActive());
    EXPECT_TRUE(handle3.isActive());
    cbArray.call();
    EXPECT_EQ(count, 2);
    handle2.remove();// This should do nothing.
    EXPECT_FALSE(cbArray.empty());
    EXPECT_FALSE(handle2.isActive());
    cbArray.call();
    EXPECT_EQ(count, 3);
    handle3.remove();
    EXPECT_TRUE(cbArray.empty());
    EXPECT_FALSE(handle3.isActive());
  }
  SECTION("CallbackSet")
  {
    Ravl2::CallbackArray<std::function<void()>> cbArray;
    EXPECT_TRUE(cbArray.empty());
    int count = 0;
    Ravl2::CallbackSet cbSet;
    cbSet += cbArray.add([&count]() { count++; });
    EXPECT_FALSE(cbArray.empty());
    cbArray.call();
    EXPECT_EQ(count, 1);
    cbSet.removeAll();
    EXPECT_TRUE(cbArray.empty());
    cbArray.call();
    EXPECT_EQ(count, 1);
  }
}

