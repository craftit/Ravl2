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

namespace Ravl2
{

  //! Type conversion for file formats.

  class TypeConverter
  {
  public:
    //! Default constructor.
    TypeConverter() = default;

    //! Constructor.
    TypeConverter(float factor,const std::type_info &from, const std::type_info &to)
        : m_factor(factor),
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
    [[nodiscard]] float factor() const noexcept
    {
      return m_factor;
    }

    //! Convert via std::any.
    [[nodiscard]] virtual std::any convert(const std::any &from) const = 0;

  private:
    float m_factor = 1.0f;
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
    void add(float cost, const ConverterT &converter)
    {
      //m_converters.emplace_back(std::make_shared<TypeConversionImpl<FromT, ToT, ConverterT>>(cost, converter));
    }

    //! Convert from one type to another.
    [[nodiscard]] std::any convert(const std::any &from, const std::type_info &to) const
    {
      (void)from;
        (void)to;
      return std::any();
    }

  private:
    //std::unordered_map<std::pair<const std::type_info &, const std::type_info &>, std::shared_ptr<TypeConverter>> m_converters;
    std::unordered_map<std::type_index, std::vector<TypeConverter>> m_converters;
  };


}// namespace Ravl2

