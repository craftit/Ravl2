#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <cereal/archives/json.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/Index.hh"
#include "Ravl2/IndexRangeSet.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))

TEST_CASE("Index")
{
  SECTION("Index 2 Cereal IO")
  {
    Ravl2::Index<2> ind1({4,6});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(ind1);
    }
    //SPDLOG_INFO("Range: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Ravl2::Index<2> ind2;
      iarchive(ind2);
      CHECK(ind1 == ind2);
    }
  }

}

TEST_CASE("IndexRange", "[IndexRange]")
{
  {
    Ravl2::IndexRange<1> range1(0,9);
    CHECK(range1.contains(1));
    CHECK_FALSE(range1.contains(10));
    int count = 0;
    for (int x: range1) {
      CHECK_EQ(x, count);
      CHECK(range1.contains(x));
      count++;
    }
    CHECK_EQ(range1.size(), count);

    Ravl2::IndexRange<1> range2(0,1);
    auto newRange = range1 - range2;
    CHECK_EQ(newRange.size(), 9);

    newRange = range1 + range2;
    CHECK_EQ(newRange.size(), 11);
  }
  {
    Ravl2::IndexRange<1> range1(2);
    CHECK(range1.size() == 1);
    CHECK(range1.min() == 2);
    CHECK(range1.max() == 2);
  }

  {
    // Check ranges not starting at 0
    Ravl2::IndexRange<1> range3(1,5);
    Ravl2::IndexRange<1> rangeRegion(-1,1);
    CHECK_EQ(range3.size(),5);
    CHECK_EQ(rangeRegion.size(),3);

    auto shrinkRange = range3.shrink(rangeRegion);
    CHECK_EQ(shrinkRange.size(),3);

    auto newRange2 = range3 - rangeRegion;
    CHECK_EQ(newRange2.size(),3);
  }

  SECTION("IndexRange<2> Iterators")
  {
    Ravl2::IndexRange<2> range2B{{1,2},{3,4}};
    CHECK_EQ(range2B.area(),4);
    auto iter = range2B.begin();
    CHECK(iter.valid());
    CHECK(!iter.done());
    Ravl2::Sentinel sent;

    for(int i = 0;i < 4;++i) {
      CHECK(iter.valid());
      CHECK(!iter.done());
      CHECK(sent != iter);
      CHECK(!(sent == iter));
      ++iter;
    }
    CHECK(!iter.valid());
    CHECK(iter.done());
    CHECK(sent == iter);
    CHECK(!(sent != iter));
  }


  SECTION("IndexRange<2> Iteration")
  {
    Ravl2::IndexRange<2> range2A{5,7};
    CHECK_EQ(range2A[0].size(),5);
    CHECK_EQ(range2A[0].min(),0);
    CHECK_EQ(range2A[0].max(),4);

    CHECK_EQ(range2A[1].size(),7);
    CHECK_EQ(range2A[1].min(),0);
    CHECK_EQ(range2A[1].max(),6);

    //std::cout << "Range: " << range2A << " \n";
    size_t count = 0;
    for(auto at : range2A) {
      count++;
      //std::cout << at << " \n";
      CHECK(range2A.contains(at));
    }
    CHECK_EQ(range2A.elements(),count);


    Ravl2::IndexRange<2> range2({2,3});
    auto newRange = range2A - range2;
    CHECK_EQ(newRange.elements(),4 * 5);

    newRange = range2A + range2;
    CHECK_EQ(newRange.elements(),6 * 9);
  }

  {
    Ravl2::IndexRange<2> range2B{{1,5},{3,7}};
    CHECK_EQ(range2B[0].size(),5);
    CHECK_EQ(range2B[0].min(),1);
    CHECK_EQ(range2B[0].max(),5);

    CHECK_EQ(range2B[1].size(),5);
    CHECK_EQ(range2B[1].min(),3);
    CHECK_EQ(range2B[1].max(),7);

    //std::cout << "Range: " << range2B << " \n";
    size_t count = 0;
    for(auto at : range2B) {
      count++;
      //std::cout << at << " \n";
      CHECK(range2B.contains(at));
    }
    CHECK_EQ(range2B.elements(),count);

    Ravl2::IndexRange<2> range2({{-1,1},{-1,1}});
    //SPDLOG_INFO("Range: {}", range2);
    auto newRange = range2B - range2;
    CHECK_EQ(newRange.elements(),9);

  }

  SECTION( "Check overlap handing in 1d  ")
  {
    Ravl2::IndexRange<1> range1(4,6);
    Ravl2::IndexRange<1> range2(3,5);
    CHECK(range1.overlaps(range2));
    CHECK(range2.overlaps(range1));

    Ravl2::IndexRange<1> range3(7,8);
    CHECK_FALSE(range1.overlaps(range3));
    CHECK_FALSE(range3.overlaps(range1));

    Ravl2::IndexRange<1> range4(1,3);
    CHECK_FALSE(range1.overlaps(range4));
    CHECK_FALSE(range4.overlaps(range1));

    CHECK(range1.overlaps(range1));

    CHECK(range4.overlaps(range2));
    CHECK(range2.overlaps(range4));

    Ravl2::IndexRange<1> range5(3,7);
    CHECK(range2.overlaps(range5));
    CHECK(range5.overlaps(range2));
  }

  SECTION("IndexRange 1 Cereal IO")
  {
    Ravl2::IndexRange<1> range1(4,6);
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(range1);
    }
    //SPDLOG_INFO("Range: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Ravl2::IndexRange<1> range2;
      iarchive(range2);
      CHECK(range1 == range2);
    }
  }

  SECTION("IndexRange 2 Cereal IO")
  {
    Ravl2::IndexRange<2> range1({{4,6},{7,9}});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(range1);
    }
    //SPDLOG_INFO("Range: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Ravl2::IndexRange<2> range2;
      iarchive(range2);
      CHECK(range1 == range2);
    }
  }
}


TEST_CASE("IndexRangeSet", "[IndexRangeSet]")
{
  using namespace Ravl2;
  SECTION("Basic 2d RangeSet operations")
  {
    IndexRange<2> rect1({{0, 1}, {0, 1}});
    IndexRange<2> rect2({{1, 2}, {1, 2}});
    //cout << "R1:" << rect1 << " Area:" << rect1.area() << "\n";
    //cout << "R2:" << rect2 << " Area:" << rect2.area() << "\n";

    IndexRangeSet<2> t1 = IndexRangeSet<2>::subtract(rect1, rect2);
    CHECK(t1.area() == 3);

    IndexRangeSet<2> t2 = IndexRangeSet<2>::subtract(rect2, rect1);
    CHECK(t2.area() == 3);

    IndexRange<2> rect3({{0, 1}, {2, 3}});

    IndexRangeSet<2> t3 = IndexRangeSet<2>::subtract(rect2, rect3);
    CHECK(t3.area() == 3);

    IndexRangeSet<2> t4 = IndexRangeSet<2>::subtract(rect3, rect2);
    CHECK(t4.area() == 3);
  }

  SECTION("Random 1d RangeSet operations")
  {
    // Add a random set of 1d ranges.
    std::random_device rd;
    // Use catch2 random number generator.
    std::mt19937 gen(rd());
    int maxRange = 30;
    std::uniform_int_distribution<> dis(0, maxRange);
    for(int k = 0;k < 100;k++) {
      IndexRangeSet<1> rngSet;
      std::vector<bool> done(size_t(maxRange), false);
      for(int i = 0;i < 100;i++) {
        int start = dis(gen);
        int finish = dis(gen);
        if(start > finish) {
          std::swap(start,finish);
        }
        IndexRange<1> rng(start,finish);
        for(auto ind : rng)
          done[size_t(ind)] = true;
        //SPDLOG_TRACE("Adding {} ",ind);

        // Add to set.
        rngSet = rngSet.add(rng);

        // Did it work ?
        CHECK(rngSet.contains(IndexRange<1>(rng)));

        // Check invariants.
        auto end = rngSet.end();
        for(auto it = rngSet.begin();it != end;it++) {
          CHECK(it->min() <= it->max());
          for(auto it2 = it+1;it2 != end;it2++) {
            CHECK_FALSE(it->overlaps(*it2));
          }
        }

        // Check logically correct.
        for(int j = 0;j < maxRange;j++) {
          CHECK(rngSet.contains(IndexRange<1>(j)) == done[size_t(j)]);
        }
      }
    }
  }
  SECTION("IndexRangeSet 2 Cereal IO")
  {
    IndexRangeSet<2> rangeSet;
    rangeSet = rangeSet.add(IndexRange<2>({{4,6},{7,9}}));
    rangeSet = rangeSet.add(IndexRange<2>({{1,3},{4,6}}));

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(rangeSet);
    }
    //SPDLOG_INFO("RangeSet: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      IndexRangeSet<2> rangeSet2;
      iarchive(rangeSet2);
      CHECK(rangeSet == rangeSet2);
    }
  }

}
