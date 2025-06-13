//
// Created by charles on 23/08/24.
//

#pragma once

#include <typeinfo>
#include <typeindex>
#include <any>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_set>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Base class for a type conversion.
  //! The 'loss' is a value between 0 and 1 that represents the loss of bits in the conversion. 0.5 would represent a 50% loss of bits.

  class TypeConverter
  {
  public:
    //! Default constructor.
    TypeConverter() = default;

    //! Constructor.
    //! @param factor - The conversion loss factor, between 0 and 1.  1-Meaning no loss, 0 meaning all information is lost.
    //! @param to - The target type.
    //! @param from - The source type.
    TypeConverter(float factor, const std::type_info &to, const std::type_info &from)
        : m_conversionLoss(factor),
          m_from(from),
          m_to(to)
    {
      // There is no point in a conversion that looses all the information, but we can't create information.
      // so the loss should be greater than 0 and less or equal to 1.
      assert(factor > 0.0f && factor <= 1.0f);
    }

    //! Make destructor virtual.
    virtual ~TypeConverter() = default;

    //! Get the source type.
    [[nodiscard]] const std::type_info &from() const noexcept
    {
      return m_from;
    }

    //! Get the target type.
    [[nodiscard]] const std::type_info &to() const noexcept
    {
      return m_to;
    }

    //! Get the conversion factor.
    //! @return conversion loss factor, between 0 and 1.  1-Meaning no loss, 0 meaning all information is lost.
    [[nodiscard]] float conversionLoss() const noexcept
    {
      return m_conversionLoss;
    }

    //! Convert types via std::any.
    //! @param from - The source type, it must be of the type specified by from().
    //! @return The converted type which will be of the type specified by to().
    [[nodiscard]] virtual std::any convert(const std::any &from) const = 0;

  private:
    float m_conversionLoss = 1.0f;
    const std::type_info &m_from = typeid(void);
    const std::type_info &m_to = typeid(void);
  };

  //! Type conversion implementation.

  template <typename ToT, typename FromT, typename CallableT>
  class TypeConversionImpl : public TypeConverter
  {
  public:
    //! Default constructor.
    TypeConversionImpl() = default;

    //! Constructor.
    explicit TypeConversionImpl(CallableT &&converter, float cost)
        : TypeConverter(cost, typeid(ToT), typeid(FromT)),
          m_converter(std::move(converter))
    {}

    //! Constructor.
    explicit TypeConversionImpl(const CallableT &converter, float cost)
        : TypeConverter(cost, typeid(ToT), typeid(FromT)),
          m_converter(converter)
    {}

    //! Convert via std::any.
    [[nodiscard]] std::any convert(const std::any &from) const final
    {
      return m_converter(std::any_cast<FromT>(from));
    }

  private:
    CallableT m_converter;
  };

  //! Type conversion chain.
  //! This provides a way to chain together a series of type conversions.

  struct ConversionChain {
  public:
    ConversionChain()
        : mEndAt(typeid(void))
    {}

    ConversionChain(std::vector<std::shared_ptr<TypeConverter>> &&chain, std::type_index endAt, float loss)
        : mChain(std::move(chain)),
          mEndAt(endAt),
          mLoss(loss)
    {}

    //! @brief Convert from one type to another.
    //! @param from - The source type which must be of the type returned by from().
    //! @return The converted type which will be of the type returned by to().
    [[nodiscard]] std::any convert(const std::any &from) const;

    //! Convert via operator().
    [[nodiscard]] std::any operator()(const std::any &from) const
    {
      return convert(from);
    }

    //! Get the conversion loss for this chain of conversions.
    //! @return An estimate of the conversion loss between 0 and 1.  1-Meaning no loss, 0 meaning all information is lost.
    [[nodiscard]] float conversionLoss() const
    {
      return mLoss;
    }

    //! Get the source type.
    //! There must be at least one conversion in the mChain.
    [[nodiscard]] const std::type_info &from() const noexcept
    {
      assert(!mChain.empty());
      return mChain.front()->from();
    }

    //! Get the target type.
    [[nodiscard]] const auto &to() const noexcept
    {
      return mEndAt;
    }

    //! Get the number of conversions in the mChain.
    [[nodiscard]] size_t size() const noexcept
    {
      return mChain.size();
    }

    //! Append a conversion to the mChain and return a new mChain.
    [[nodiscard]] ConversionChain append(std::shared_ptr<TypeConverter> converter) const
    {
      assert(converter != nullptr);
      assert(converter->from() == mEndAt);
      std::vector<std::shared_ptr<TypeConverter>> newChain = mChain;
      auto const &toType = converter->to();
      auto newLoss = mLoss * converter->conversionLoss();
      newChain.emplace_back(std::move(converter));
      return {std::move(newChain), toType, newLoss};
    }

    //! Prepend a conversion to the mChain and return a new mChain.
    [[nodiscard]] ConversionChain prepend(std::shared_ptr<TypeConverter> converter) const
    {
      assert(converter != nullptr);
      assert(converter->to() == from());
      std::vector<std::shared_ptr<TypeConverter>> newChain;
      auto newLoss = mLoss * converter->conversionLoss();
      newChain.reserve(mChain.size() + 1);
      newChain.push_back(std::move(converter));
      newChain.insert(newChain.end(), mChain.begin(), mChain.end());
      return {std::move(newChain), mEndAt, newLoss};
    }

  private:
    std::vector<std::shared_ptr<TypeConverter>> mChain;
    std::type_index mEndAt;//!< We need to store this as the last conversion may be empty.
    float mLoss = 1.0f;
  };

  //! Helper to make a type conversion.
  template <typename ToT, typename FromT, typename CallableT>
  std::shared_ptr<TypeConversionImpl<ToT, FromT, CallableT>> makeTypeConversion(CallableT &&converter, float cost)
  {
    return std::make_shared<TypeConversionImpl<ToT, FromT, CallableT>>(std::forward<CallableT>(converter), cost);
  }

  //! Set of type conversions for use with file formats.

  class TypeConverterMap
  {
  public:
    //! Default constructor.
    TypeConverterMap() = default;

    //! Add a type conversion.
    template <typename ToT, typename FromT, typename CallableT>
    bool add(CallableT &&converter, float cost)
    {
      auto x = makeTypeConversion<ToT, FromT, CallableT>(std::forward<CallableT>(converter), cost);
      std::lock_guard lock(m_mutex);
      m_converters[std::type_index(x->from())].push_back(x);
      m_conversionCache.clear();
      mVersion++;
      return true;
    }

    //! Add a type conversion.
    template <typename ToT, typename FromT, typename CallableT>
    bool add(const CallableT &converter, float cost)
    {
      auto x = std::make_shared<TypeConversionImpl<ToT, FromT, CallableT>>(converter, cost);
      std::lock_guard lock(m_mutex);
      m_converters[std::type_index(typeid(FromT))].push_back(x);
      m_conversionCache.clear();
      mVersion++;
      return true;
    }

    //! Dump the conversion map.
    void dump();

    //! Find a convertion mChain with a best first search.
    [[nodiscard]] std::optional<ConversionChain> find(const std::type_info &to, const std::type_info &from);

    //! Find a convertion mChain to one of a set of possible types with a best first search.
    [[nodiscard]] std::optional<ConversionChain> find(const std::unordered_set<std::type_index> &to, const std::type_info &from);

    //! Find a convertion mChain to one of set of possible types with a best first search.
    [[nodiscard]] std::optional<ConversionChain> find(const std::type_index &to, const std::unordered_set<std::type_index> &from);

    //! Convert from one type to another.
    [[nodiscard]] std::optional<std::any> convert(const std::type_info &to, const std::any &from);

  private:
    //! Find a convertion mChain to one of a set of possible types with a best first search.
    [[nodiscard]] std::optional<ConversionChain> findInternal(const std::unordered_set<std::type_index> &to, const std::unordered_set<std::type_index> &from);

    std::shared_mutex m_mutex;
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<TypeConverter>>> m_converters;
    struct CacheKey {
      CacheKey() = default;
      CacheKey(const std::unordered_set<std::type_index> &from, const std::unordered_set<std::type_index> &to)
          : mFrom(from),
            mTo(to)
      {}

      bool operator==(const CacheKey &other) const
      {
        return mFrom == other.mFrom && mTo == other.mTo;
      }

      bool operator!=(const CacheKey &other) const
      {
        return !(*this == other);
      }

      std::unordered_set<std::type_index> mFrom;
      std::unordered_set<std::type_index> mTo;
    };
    struct Hash {
      size_t operator()(const CacheKey &key) const
      {
	size_t hash = 7;
	for(const auto &x : key.mFrom)
	  hash ^= std::hash<std::type_index>()(x);
	hash *= 31;
        for(const auto &x : key.mTo)
          hash ^= std::hash<std::type_index>()(x);
        return hash;
      }
    };
    std::unordered_map<CacheKey, std::optional<ConversionChain>, Hash> m_conversionCache;
    size_t mVersion = 0;
  };

  TypeConverterMap &typeConverterMap();

  namespace detail
  {
    // Helper to get type of function arguments and return type.
    template <typename T>
    struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>> {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    // Class member function
    template <typename C, typename R, typename... Args>
    struct function_traits<R (C::*)(Args...)> {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R (C::*)(Args...) const> {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    // Deal with function pointers
    template <typename R, typename... Args>
    struct function_traits<R (*)(Args...)> {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    template <typename R, typename... Args>
    struct function_traits<R (&)(Args...)> {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    // Deal with lambda types
    template <typename T>
      requires std::is_class_v<T>
    struct function_traits<T> : public function_traits<decltype(&T::operator())> {
    };
  }// namespace detail

  //! Register a converter based on a lambda.
  template <typename FuncT, typename OperatorT = decltype(&FuncT::operator())>
    requires std::is_class_v<FuncT> && (std::tuple_size<typename detail::function_traits<OperatorT>::argument_type>::value == 1)
  bool registerConversion(FuncT &&callable, float cost)
  {
    // We need to look at the signature of the lambda operator() to get the types.
    using ToT = typename detail::function_traits<OperatorT>::result_type;
    using FromT = std::tuple_element_t<0, typename detail::function_traits<OperatorT>::argument_type>;
    typeConverterMap().add<ToT, std::decay_t<FromT>, FuncT>(std::forward<FuncT>(callable), cost);
    return true;
  }

  //! Register a converter based on a function pointer.
  template <typename ToT, typename FromT>
  bool registerConversion(ToT (*func)(FromT), float cost)
  {
    typeConverterMap().add<ToT, std::decay_t<FromT>, ToT (*)(FromT)>(func, cost);
    return true;
  }

  //! Convert from one type to another using the default type converter map.
  //! This mechanism is intended for use with file formats where code may not have access to the types.
  template <typename ToT, typename FromT>
  ToT typeConvert(const FromT &from)
  {
    if constexpr(std::is_same_v<ToT, std::decay_t<FromT>>)
      return from;
    if constexpr(std::is_convertible_v<FromT, ToT>)
      return static_cast<ToT>(from);
    auto x = typeConverterMap().convert(typeid(ToT), from);
    if(!x.has_value()) {
      SPDLOG_WARN("Don't know how to convert {} to {}", typeName(typeid(FromT)), typeName(typeid(ToT)));
      throw std::runtime_error("Failed to convert type");
    }
    return std::any_cast<ToT>(x.value());
  }

  //! Convert from a std::any to another using the default type converter map.
  //! This mechanism is intended for use with file formats where code may not have access to the types.
  template <typename ToT>
  ToT typeConvert(const std::any &from)
  {
    if(from.type() == typeid(ToT)) {
      return std::any_cast<ToT>(from);
    }
    auto x = typeConverterMap().convert(typeid(ToT), from);
    if(!x.has_value()) {
      SPDLOG_WARN("Don't know how to convert {} to {}", typeName(from.type()), typeName(typeid(ToT)));
      throw std::runtime_error("Failed to convert type");
    }
    return std::any_cast<ToT>(x.value());
  }

}// namespace Ravl2
