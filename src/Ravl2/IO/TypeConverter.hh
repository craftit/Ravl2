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
#include <deque>
#include <map>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <spdlog/spdlog.h>
//#include "Ravl2/function_traits.hh"

namespace Ravl2
{

  //! Abstract type conversion for use with file formats.

  class TypeConverter
  {
  public:
    //! Default constructor.
    TypeConverter() = default;

    //! Constructor.
    TypeConverter(float factor,const std::type_info &from, const std::type_info &to)
        : m_conversionLoss(factor),
          m_from(from),
          m_to(to)
    {}

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
    [[nodiscard]] float conversionLoss() const noexcept
    {
      return m_conversionLoss;
    }

    //! Convert via std::any.
    [[nodiscard]] virtual std::any convert(const std::any &from) const = 0;

  private:
    float m_conversionLoss = 1.0f;
    const std::type_info &m_from = typeid(void);
    const std::type_info &m_to = typeid(void);
  };


  //! Type conversion implementation.

  template <typename ToT, typename FromT,typename CallableT>
  class TypeConversionImpl : public TypeConverter
  {
  public:
    //! Default constructor.
    TypeConversionImpl() = default;

    //! Constructor.
    explicit TypeConversionImpl(CallableT &&converter,float cost)
      : TypeConverter(cost, typeid(FromT), typeid(ToT)),
        m_converter(std::move(converter))
    {}

    //! Constructor.
    explicit TypeConversionImpl(const CallableT &converter,float cost)
        : TypeConverter(cost, typeid(FromT), typeid(ToT)),
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

  //! Type conversion

  class TypeConverterMap
  {
  public:
    //! Default constructor.
    TypeConverterMap() = default;

    //! Add a type conversion.
    template <typename ToT, typename FromT,typename CallableT>
    bool add(CallableT &&converter, float cost)
    {
      auto x = std::make_shared<TypeConversionImpl<ToT, FromT, CallableT>>(std::forward<CallableT>(converter),cost);
      m_converters[std::type_index(typeid(FromT))].push_back(x);
      return true;
    }

    //! Add a type conversion.
    template <typename ToT, typename FromT,typename CallableT>
    bool add(const CallableT &converter,float cost)
    {
      auto x = std::make_shared<TypeConversionImpl<ToT, FromT, CallableT>>(converter,cost);
      m_converters[std::type_index(typeid(FromT))].push_back(x);
      return true;
    }


    //! Find a convertion chain, with a best first search.
    [[nodiscard]] std::optional<std::vector<std::shared_ptr<TypeConverter>>> find(const std::type_info &from, const std::type_info &to) const;

    //! Convert from one type to another.
    [[nodiscard]] std::optional<std::any> convert(const std::any &from, const std::type_info &to) const;

  private:
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<TypeConverter>>> m_converters;
  };

  TypeConverterMap &typeConverterMap();

  namespace detail
  {
    // Helper to get type of a function arguments and return type.
    template <typename T>
    struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>>
    {
        using result_type = R;
        using argument_type = std::tuple<Args...>;
    };

    // Class member function
    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)>
    {
        using result_type = R;
        using argument_type = std::tuple<Args...>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const>
    {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    // Deal with function pointers
    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)>
    {
        using result_type = R;
        using argument_type = std::tuple<Args...>;
    };

    template <typename R, typename... Args>
    struct function_traits<R(&)(Args...)>
    {
      using result_type = R;
      using argument_type = std::tuple<Args...>;
    };

    // Deal with lambda types
    template <typename T>
     requires std::is_class_v<T>
    struct function_traits<T>
      : public function_traits<decltype(&T::operator())>
    {};
  }

  //! Register a converter based on a lambda.
  template <typename FuncT, typename OperatorT = decltype(&FuncT::operator())>
    requires std::is_class_v<FuncT> && (std::tuple_size<typename detail::function_traits<OperatorT>::argument_type>::value == 1)
  bool registerConversion(FuncT &&callable,float cost)
  {
    // We need to look at the signature of the lambda operator() to get the types.
    using ToT = typename detail::function_traits<OperatorT>::result_type;
    using FromT = std::tuple_element_t<0,typename detail::function_traits<OperatorT>::argument_type>;
    typeConverterMap().add<ToT,std::decay_t<FromT>,FuncT>(std::forward<FuncT>(callable), cost);
    return true;
  }

  //! Register a converter based on a function pointer.
  template <typename ToT,typename FromT>
  bool registerConversion(ToT (*func)(FromT),float cost)
  {
    typeConverterMap().add<ToT,std::decay_t<FromT>,ToT (*)(FromT)>(func, cost);
    return true;
  }

}// namespace Ravl2

