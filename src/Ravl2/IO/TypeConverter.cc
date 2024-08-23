//
// Created by charles on 23/08/24.
//

#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  TypeConverterMap &typeConverterMap()
  {
    static TypeConverterMap instance;
    return instance;
  }

  std::optional<std::vector<std::shared_ptr<TypeConverter>>> TypeConverterMap::find(const std::type_info &from, const std::type_info &to) const
  {
    // Best first search for a chain of converters.
    if(from == to)
    {
      return std::vector<std::shared_ptr<TypeConverter>>{};
    }
    struct ChainEntryT
    {
      std::vector<std::shared_ptr<TypeConverter>> chain;
      std::type_index from;
      float loss = 1.0f; //1.0f is no loss
    };
    std::multimap<float,ChainEntryT> solutions;
    std::unordered_map<std::type_index, float> visited;
    visited[std::type_index(from)] = 1.0f;

    solutions.emplace(1.0f,ChainEntryT{{},std::type_index(from),1.0f});

    while(!solutions.empty())
    {
      auto [loss,entry] = *solutions.begin();
      solutions.erase(solutions.begin());
      if(entry.from == std::type_index(to))
      {
        return entry.chain;
      }
      for(auto &x : m_converters.at(entry.from))
      {
        // Note we want to avoid loss, so higher loss is better.
        auto newLoss = loss * x->conversionLoss() * 0.999f; // Prefer shorter chains
        if(newLoss > visited[std::type_index(x->to())])
        {
          visited[std::type_index(x->to())] = newLoss;
          auto newEntry = entry;
          newEntry.chain.push_back(x);
          newEntry.from = std::type_index(x->to());
          newEntry.loss = newLoss;
          solutions.emplace(newLoss,newEntry);
        }
      }
    }
    return std::nullopt;
  }

  std::optional<std::any> TypeConverterMap::convert(const std::any &from, const std::type_info &to) const
  {
    const auto& fromType = from.type();
    if(fromType == to)
    {
      return from;
    }
    auto converters = find(fromType, to);
    if(!converters.has_value())
    {
      return std::nullopt;
    }
    auto result = from;
    for(auto &x : converters.value())
    {
      result = x->convert(result);
    }
    return result;
  }
}// namespace Ravl2