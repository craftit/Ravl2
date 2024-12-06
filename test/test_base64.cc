
#include <random>
#include "catch2checks.hh"
#include "Ravl2/Base64.hh"


TEST_CASE("base64")
{
  //std::random_device dev;
  //std::mt19937 rng(dev());
  std::mt19937 rng;
  std::uniform_int_distribution<uint8_t> distBytes(0, 255);
  std::uniform_int_distribution<size_t> distLengths(0, 400);
  spdlog::set_level(spdlog::level::off);

  for(int i = 0; i < 100; i++) {
    std::vector<std::byte> data(distLengths(rng));
    for(auto &d : data) {
      d = std::byte(distBytes(rng));
    }
    std::string const enc = fmt::format("\n {}\n", Ravl2::base64Encode(data));
    std::vector<std::byte> data2 = Ravl2::base64Decode(enc);
    ASSERT_EQ(data2.size(), data.size());
    // Check the data int data and data2 are the same.
    for(size_t k = 0; k < data.size(); k++) {
      if(data[k] != data2[k]) {
        SPDLOG_ERROR("Mismatch at {} : {} != {}", k, int(data[k]), int(data2[k]));
      }
      ASSERT_EQ(data[k], data2[k]);
    }
  }

  // Check some corner cases.
  {
    std::string testStr("");
    std::vector<std::byte> dat = Ravl2::base64Decode(testStr);
    ASSERT_EQ(dat.size(), 0);

    testStr = "====";
    dat = Ravl2::base64Decode(testStr);
    ASSERT_EQ(dat.size(), 1);
  }

  // Check we throw an exception on invalid strings.
  std::string sourceStr = "ABCD";
  for(size_t i = 1; i < 4; i++) {
    try {
      // Pass an invalid strings
      std::string enc;
      for(size_t k = 0; k < i; k++)
        enc += sourceStr[k];
      auto dat = Ravl2::base64Decode(enc);
      // It should throw an exception
      ASSERT_TRUE(false);
    } catch(...) {
    }
  }

  // Throw rubbish at the decoder.
  // Is should either return data, or throw an exception.
  std::uniform_int_distribution<size_t> distStrings(0, 2049);

  unsigned datCount = 0, excCount = 0;
  for(int i = 0; i < 10000; i++) {
    std::string edata;
    size_t len = distStrings(rng);
    edata.reserve(len);
    for(unsigned k = 0; k < len; k++)
      edata += static_cast<char>(distBytes(rng));
    try {
      std::vector<std::byte> dat = Ravl2::base64Decode(edata);
      datCount++;
    } catch(...) {
      excCount++;
    }
  }
  SPDLOG_INFO("base64Decode: {} data, {} exceptions", datCount, excCount);
}