//
// Created by Charles Galambos on 15/05/2021.
//

#include "checks.hh"
#include <algorithm>
#include <cstdlib>
#include "Ravl2/RankedScoreArray.hh"

TEST_CASE("RankedScoreArray")
{
  const size_t maxSize = 10;
  Ravl2::RankedScoreArray<int,int> test(maxSize);
  test.setMaxSize(maxSize);
  ASSERT_EQ(test.size(), 0);

  //SPDLOG_INFO("Adding items");
  for(size_t i = 0;i < (maxSize*2);i+=2) {
    //SPDLOG_INFO("Adding {} ",i);
    test.insert(int(i), int(i));
    ASSERT_EQ(test.size(), std::min((i/2)+1,maxSize));
  }

  // Check the ranking of the scores
  {
    int lastScore = std::get<0>(test[0]);
    for (size_t i = 1; i < test.size(); i++) {
      CHECK(std::get<0>(test[i]) > lastScore);
    }
  }
  //SPDLOG_INFO("Dumping.");
  //test.dump(std::cout);

  //SPDLOG_INFO("Dumping.");
  test.insert(9,100);

  //test.dump(std::cout);

  test.insert(5,100);

  //test.dump(std::cout);
  //SPDLOG_INFO("Random numbers ");
  for(int i =0;i < 100;i++) {
    test.insert(rand() % 100,200+i);
  }
  ASSERT_EQ(test.size(), maxSize);

  // Check the ranking of the scores
  {
    int lastScore = std::get<0>(test[0]);
    for (size_t i = 1; i < test.size(); i++) {
      CHECK(std::get<0>(test[i]) >= lastScore);
    }
  }
  //SPDLOG_INFO("Dumping.");
  //test.dump(std::cout);
}

