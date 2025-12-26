//
// Created by charles on 23/08/24.
//

#include <shared_mutex>
#include "Ravl2/IO/TypeConverter.hh"

#include "Ravl2/Assert.hh"

#include <map>

namespace Ravl2
{
  TypeConverterMap &typeConverterMap()
  {
    static TypeConverterMap instance;
    return instance;
  }

  std::optional<ConversionChain> TypeConverterMap::findInternal(const std::unordered_set<std::type_index> &to, const std::unordered_set<std::type_index> &from)
  {
    std::shared_lock lock(m_mutex);
    auto startVersion = mVersion;
    CacheKey cacheKey = {from, to};
    auto it = m_conversionCache.find(cacheKey);
    if(it != m_conversionCache.end()) {
      return it->second;
    }

    std::multimap<float, ConversionChain> solutions;
    std::unordered_map<std::type_index, float> visited;
    for(const auto &x : from) {
      visited[x] = 1.0f;
      solutions.emplace(1.0f, ConversionChain {{}, x, 1.0f});
    }

    while(!solutions.empty()) {
      auto [loss, entry] = *solutions.begin();
      solutions.erase(solutions.begin());
      if(to.contains(entry.to())) {
        lock.unlock();
        SPDLOG_TRACE("Found conversion from '{}' to '{}' with {} steps. ", typeName(from), typeName(entry.to()), entry.size());
        std::lock_guard writeLock(m_mutex);
        // If the version has not changed by another thread, we can update the cache.
        if(startVersion == mVersion) {
          m_conversionCache.emplace(cacheKey, entry);
        }
        return entry;
      }
      auto fromList = m_converters.find(entry.to());
      if(fromList == m_converters.end()) {
        continue;
      }
      for(auto &x : fromList->second) {
        // Note we want to avoid mLoss, so higher mLoss is better.
        if(!x) {
          SPDLOG_WARN("Null converter found in list for {}", typeName(entry.to()));
          continue;
        }
        auto newLoss = loss * x->conversionLoss() * 0.999f;// Prefer shorter chains
        if(newLoss > visited[std::type_index(x->to())]) {
          visited[std::type_index(x->to())] = newLoss;
          solutions.emplace(newLoss, entry.append(x));
        }
      }
    }
    lock.unlock();
    {
      std::lock_guard writeLock(m_mutex);
      // If the version has not changed by another thread, we can update the cache.
      if(startVersion == mVersion) {
        m_conversionCache.emplace(cacheKey, std::nullopt);
      }
    }
    SPDLOG_TRACE("Failed to find conversion from {} to {}", typeName(from), typeName(*to.begin()));
    return std::nullopt;
  }

  std::optional<ConversionChain> TypeConverterMap::find(const std::type_info &to, const std::type_info &from)
  {
    return findInternal({to}, {from});
  }

  //! Find a convertion mChain to one of set of possible types with a best first search.
  [[nodiscard]] std::optional<ConversionChain> TypeConverterMap::find(const std::unordered_set<std::type_index> &to, const std::type_info &from)
  {
    return typeConverterMap().findInternal(to, {from});
  }

  //! Find a convertion mChain to one of set of possible types with a best first search.
  [[nodiscard]] std::optional<ConversionChain> TypeConverterMap::find(const std::type_index &to, const std::unordered_set<std::type_index> &from)
  {
    return typeConverterMap().findInternal({to}, from);
  }

  std::optional<std::any> TypeConverterMap::convert(const std::type_info &to, const std::any &from)
  {
    const auto &fromType = from.type();
    if(fromType == to) {
      return from;
    }
    auto converters = find(to, fromType);
    if(!converters.has_value()) {
      return std::nullopt;
    }
    return converters.value().convert(from);
  }

  void TypeConverterMap::dump()
  {
    std::shared_lock lock(m_mutex);
    SPDLOG_INFO("TypeConverterMap. Converters: {} ", m_converters.size());
    for(const auto &x : m_converters) {
      SPDLOG_INFO("From: {}", typeName(x.first));
      for(const auto &y : x.second) {
        SPDLOG_INFO("  To: {}", typeName(y->to()));
      }
    }
  }

  namespace
  {
    // These are used to register conversions, they are not used directly. They current content
    // is as much for checking the compiler is happy constructing the function as anything else.

    [[maybe_unused]] bool g_reg = registerConversion([](float val) { return double(val); }, 1.0f);

    int32_t func(int16_t x)
    { return x; }
    [[maybe_unused]] bool g_reg2 = registerConversion(func, 1.0f);

    int64_t func2(const int32_t &x)
    { return x; }
    [[maybe_unused]] bool g_reg3 = registerConversion(func2, 1.0f);

    float func3(const double &x)
    {  return static_cast<float>(x); }
    [[maybe_unused]] bool g_reg4 = registerConversion(func3, 0.5f);

    double func4(const float &x)
    { return static_cast<double>(x); }
    [[maybe_unused]] bool g_reg5 = registerConversion(func4, 1.0f);

    size_t func5(const int &x)
    {
      if(x < 0) {
        SPDLOG_WARN("Negative value {} converted to 0", x);
        throw std::out_of_range("Value outside valid range.");
      }
      return static_cast<size_t>(x);
    }
    [[maybe_unused]] bool g_reg6 = registerConversion(func5, 0.5f);

    unsigned func6(const int &x)
    {
      if(x < 0) {
        SPDLOG_WARN("Negative value {} converted to 0", x);
        throw std::out_of_range("Value outside valid range.");
      }
      return static_cast<unsigned>(x);
    }
    [[maybe_unused]] bool g_reg7 = registerConversion(func6, 0.5f);

    int32_t func7(const int64_t &x)
    {
      if(x < std::numeric_limits<int32_t>::min() || x > std::numeric_limits<int32_t>::max()) {
        SPDLOG_WARN("Value {} out of range for int32_t", x);
        throw std::out_of_range("Value outside valid range.");
      }
      return static_cast<int32_t>(x);
    }
    [[maybe_unused]] bool g_reg8 = registerConversion(func7, 0.5f);

    int32_t func8(const float &x)
    {
      if(x < float(std::numeric_limits<int32_t>::min()) || x > float(std::numeric_limits<int32_t>::max())) {
        SPDLOG_WARN("Value {} out of range for int32_t", x);
        throw std::out_of_range("Value outside valid range.");
      }
      return static_cast<int32_t>(x);
    }
    [[maybe_unused]] bool g_reg9 = registerConversion(func8, 0.5f);

    float func9(const long &x)
    {
      return static_cast<float>(x);
    }
    [[maybe_unused]] bool g_reg10 = registerConversion(func9, 0.8f);

    int func10(const unsigned &x)
    {
      if(x > static_cast<unsigned>(std::numeric_limits<int>::max())) {
        SPDLOG_ERROR("Value outside valid range.", x);
        throw std::out_of_range("Value outside valid range.");
      }
      return static_cast<int>(x);
    }
    [[maybe_unused]] bool g_reg11 = registerConversion(func10, 0.5f);

  }// namespace

  std::any ConversionChain::convert(const std::any &from) const
  {
    std::any x = from;
    for(auto &y : mChain) {
      x = y->convert(x);
    }
    return x;
  }
}// namespace Ravl2