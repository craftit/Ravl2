//! @file
//! @brief JSON configuration object, this provides the ability to load and save configuration from JSON files.
//! @author Charles Galambos
//! @date 07/03/2021
//! @details
//! This provides a backend to the configuration system that uses JSON files.  This is the default backend.

#pragma once

#include <utility>
#include <nlohmann/json.hpp>
#include "Ravl2/Configuration.hh"

namespace Ravl2
{

  //! JSON configuration object

  class ConfigNodeJSON : public ConfigNode
  {
  public:
    using json = nlohmann::json;

    //! Construct directly from JSON data.
    explicit ConfigNodeJSON(json data)
        : m_json(std::move(data))
    {}

    //! Construct from file
    explicit ConfigNodeJSON(std::string_view filename);

    //! Create a new child node.
    ConfigNodeJSON(ConfigNode &parent, std::string &&name, std::string &&description, std::any &&value, json jvalue)
        : ConfigNode(parent, std::move(name), std::move(description), std::move(value)),
          m_json(std::move(jvalue))
    {}

    using ConfigNode::initNumber;

    //! Initialise a number field
    std::any initNumber(const std::string_view &name, const std::string_view &description, int defaultValue, int min, int max) override;

    //! Initialise a number field
    std::any initNumber(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max) override;

    //! Initialise a number field
    std::any initNumber(const std::string_view &name, const std::string_view &description, double defaultValue, double min, double max) override;

    //! Initialise a number field
    std::any initNumber(const std::string_view &name, const std::string_view &description, size_t defaultValue, size_t min, size_t max) override;

    //! Initialise a string field
    std::any initString(const std::string_view &name, const std::string_view &description, const std::string_view &defaultValue) override;

    //! Initialise a string field
    std::any initBool(const std::string_view &name, const std::string_view &description, bool defaultValue) override;

    //! Initialise an object field
    //! @param name Name of the field
    //! @param description Description of the field
    //! @param type Type of the object
    //! @param defaultValue Default value of the field
    //! @return The value of the field
    std::any initObject(const std::string_view &name,
                        const std::string_view &description,
                        const std::type_info &type,
                        const std::string_view &defaultType) override;

    //! Get list of child nodes.
    std::vector<std::shared_ptr<ConfigNode>> getChildNodes() override;

    //! Build a child node
    std::shared_ptr<ConfigNode> makeChildNode(std::string &&name, std::string &&description,
                                              std::any &&value) override;

    //! Is the value of a field defined in the configuration file?
    [[nodiscard]] bool isDefined(const std::string_view &name) const override
    {
      return m_json.find(std::string(name)) != m_json.end();
    }

    //! Get value as string
    [[nodiscard]] std::string asString(const std::string_view &name) const override
    {
      return m_json[std::string(name)].template get<std::string>();
    }

    //! Set child value.
    std::shared_ptr<ConfigNode> setChild(std::string &&name, std::string &&description, std::any &&value) override;

    //! Do we have a child in the config, even if not created yet?
    [[nodiscard]] bool hasChild(const std::string_view &name) override;

    //! Check all subfields are used.
    void checkAllFieldsUsed() const override;

  protected:
    json m_json;
  };

}// namespace Ravl2
