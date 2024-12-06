//
// Created by charles galambos on 27/02/2022.
//

#include <thread>
#include "catch2checks.hh"
#include "Ravl2/ThreadedQueue.hh"

// Check basic TQueue functionality in a single threaded environment

TEST_CASE("ThreadedQueue")
{
  SECTION("Queue Functions")
  {
    Ravl2::ThreadedQueue<int> q;

    int v = 0;
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
    ASSERT_FALSE(q.tryPop(v));

    q.push(1);// 1

    ASSERT_FALSE(q.empty());
    ASSERT_EQ(q.size(), 1);
    ASSERT_TRUE(q.tryPop(v));
    ASSERT_EQ(v, 1);
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
    ASSERT_FALSE(q.popWait(v, 0.01f));

    for(int i = 0; i < 10; ++i)
      q.push(i);
    ASSERT_FALSE(q.tryPush(10));
    ASSERT_EQ(q.size(), 10);
  }
  SECTION("Thread safety")
  {
    //! Check TQueue functionality in a multi-threaded environment
    Ravl2::ThreadedQueue<int> q;

    int v = 0;
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
    ASSERT_FALSE(q.tryPop(v));

    std::thread t1([&q]() {
      for(int i=0; i<10; ++i) {
        q.push(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    std::thread t2([&q]() {
      for(int i=10; i<20; ++i) {
        q.push(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    std::vector<bool> vb(20,false);
    for(size_t i=0; i<20; ++i)
    {
      ASSERT_TRUE(q.popWait(v,10.0f));
      vb[size_t(v)] = true;
    }
    for (size_t i = 0; i < 20; ++i) {
      ASSERT_TRUE(vb[i]);
    }
    ASSERT_FALSE(q.popWait(v,0.1f));
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);

    t1.join();
    t2.join();


  }
}



