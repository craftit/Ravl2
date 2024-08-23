//
// Created by charles on 23/08/24.
//

#pragma once

#include <typeinfo>
#include <typeindex>
#include <any>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include <map>

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

  template <typename FromT, typename ToT, typename ConverterT>
  class TypeConversionImpl : public TypeConverter
  {
  public:
    //! Default constructor.
    TypeConversionImpl() = default;

    //! Constructor.
    explicit TypeConversionImpl(float cost,const ConverterT &converter)
        : TypeConverter(cost, typeid(FromT), typeid(ToT)),
          m_converter(converter)
    {}

    //! Get the converter.
    [[nodiscard]] const ConverterT &converter() const noexcept
    {
      return m_converter;
    }

    //! Convert via std::any.
    [[nodiscard]] std::any convert(const std::any &from) const final
    {
      return m_converter(std::any_cast<FromT>(from));
    }

  private:
    ConverterT m_converter;
  };

  //! Type conversion

  class TypeConverterMap
  {
  public:
    //! Default constructor.
    TypeConverterMap() = default;

    //! Add a type conversion.
    template <typename FromT, typename ToT, typename ConverterT>
    bool add(float cost, const ConverterT &converter)
    {
      auto x = std::make_shared<TypeConversionImpl<FromT, ToT, ConverterT>>(cost, converter);
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

  //! Register a converter.
  template <typename ConverterT, typename FromT, typename ToT = >
  bool registerTypeConverter(float cost, const ConverterT &converter)
  {
    return typeConverterMap().add<FromT, ToT, ConverterT>(cost, converter);
  }

}// namespace Ravl2

