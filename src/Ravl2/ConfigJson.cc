//
// Created by Charles Galambos on 07/03/2021.
//

#include "Ravl2/ConfigJson.hh"
#include <fstream>

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //! Load configuration from a file.
  Configuration::Configuration(const std::string_view &filename)
      : m_node(std::make_shared<ConfigNodeJSON>(filename))
  {}

  //! Load configuration from a JSON string
  Configuration Configuration::fromJSONString(const std::string_view &data)
  {
    nlohmann::json jsonData = nlohmann::json::parse(data);

    return Configuration(std::make_shared<ConfigNodeJSON>(jsonData));
  }

  // ----------------------------------------------------------

  ConfigNodeJSON::ConfigNodeJSON(std::string_view filename)
  {
    ONDEBUG(SPDLOG_INFO("Reading config file '{}' ", filename));
    std::ifstream ifs;
    ifs.open(std::string(filename));
    if(!ifs.is_open()) {
      SPDLOG_ERROR("Failed to open file '{}' ", filename);
      throw std::runtime_error("Failed to open file.");
    }
    m_json = json::parse(ifs);
  }

  std::any ConfigNodeJSON::initNumber(const std::string_view &name,
                                      const std::string_view &description,
                                      int defaultValue,
                                      int min,
                                      int max)
  {
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = defaultValue;
      return ConfigNode::initNumber(name, description, defaultValue, min, max);
    }
    json const value = m_json[tname];
    if(!value.is_number()) {
      SPDLOG_ERROR("Expected a number for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a number in field.");
    }
    int val = value.template get<int>();
    if(val < min || val > max) {
      SPDLOG_ERROR("Number for field {}.{} out of range. {} <= ({}) <= {}  ", rootPathString(), name, min, val, max);
      throw std::out_of_range("Out of range.");
    }
    auto x = setChild(std::string(name), std::string(description), val);
    return x->value();
  }

  std::any ConfigNodeJSON::initNumber(const std::string_view &name,
				      const std::string_view &description,
				      unsigned defaultValue,
				      unsigned min,
				      unsigned max)
  {
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = defaultValue;
      return ConfigNode::initNumber(name, description, defaultValue, min, max);
    }
    json const value = m_json[tname];
    if(!value.is_number()) {
      SPDLOG_ERROR("Expected a number for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a number in field.");
    }
    unsigned val = value.template get<unsigned>();
    if(val < min || val > max) {
      SPDLOG_ERROR("Number for field {}.{} out of range. {} <= ({}) <= {}  ", rootPathString(), name, min, val, max);
      throw std::out_of_range("Out of range.");
    }
    auto x = setChild(std::string(name), std::string(description), val);
    return x->value();
  }


  std::any ConfigNodeJSON::initNumber(const std::string_view &name,
                                      const std::string_view &description,
                                      float defaultValue,
                                      float min,
                                      float max)
  {
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = static_cast<double>(defaultValue);
      return ConfigNode::initNumber(name, description, defaultValue, min, max);
    }
    json const value = m_json[tname];
    if(!value.is_number()) {
      SPDLOG_ERROR("Expected a number for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a number in field.");
    }
    float val = value.template get<float>();
    if(val < min || val > max) {
      SPDLOG_ERROR("Number for field {}.{} out of range. {} <= ({}) <= {}  ", rootPathString(), name, min, val, max);
      throw std::out_of_range("Out of range.");
    }
    auto x = setChild(std::string(name), std::string(description), val);
    return x->value();
  }

  std::any ConfigNodeJSON::initNumber(const std::string_view &name,
                                      const std::string_view &description,
                                      double defaultValue,
                                      double min,
                                      double max)
  {
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = static_cast<double>(defaultValue);
      return ConfigNode::initNumber(name, description, defaultValue, min, max);
    }
    json const value = m_json[tname];
    if(!value.is_number()) {
      SPDLOG_ERROR("Expected a number for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a number in field.");
    }
    double val = value.template get<double>();
    if(val < min || val > max) {
      SPDLOG_ERROR("Number for field {}.{} out of range. {} <= ({}) <= {}  ", rootPathString(), name, min, val, max);
      throw std::out_of_range("Out of range.");
    }
    auto x = setChild(std::string(name), std::string(description), val);
    return x->value();
  }

  std::any
  ConfigNodeJSON::initNumber(const std::string_view &name, const std::string_view &description, size_t defaultValue, size_t min, size_t max)
  {
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = defaultValue;
      return ConfigNode::initNumber(name, description, defaultValue, min, max);
    }
    json const value = m_json[tname];
    if(!value.is_number()) {
      SPDLOG_ERROR("Expected a number for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a number in field.");
    }
    size_t val = value.template get<size_t>();
    if(val < min || val > max) {
      SPDLOG_ERROR("Number for field {}.{} out of range. {} <= ({}) <= {}  ", rootPathString(), name, min, val, max);
      throw std::out_of_range("Out of range.");
    }
    auto x = setChild(std::string(name), std::string(description), val);
    return x->value();
  }

  std::any ConfigNodeJSON::initString(const std::string_view &name,
                                      const std::string_view &description,
                                      const std::string_view &defaultValue)
  {
    ONDEBUG(SPDLOG_INFO("initString for {} ", name));
    std::string const tname(name);// jsoncpp doesn't support string_view.
    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = std::string(defaultValue);
      return ConfigNode::initString(name, description, defaultValue);
    }
    json const value = m_json[tname];
    if(!value.is_string()) {
      SPDLOG_ERROR("Expected a string for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a string in field.");
    }
    auto x = setChild(std::string(name), std::string(description), value.template get<std::string>());
    return x->value();
  }

  std::any ConfigNodeJSON::initBool(const std::string_view &name,
                                    const std::string_view &description,
                                    bool defaultValue)
  {
    std::string tname(name);// jsoncpp doesn't support string_view.

    // Just use the default value?
    if(m_json.find(tname) == m_json.end()) {
      m_json[tname] = defaultValue;
      return ConfigNode::initBool(name, description, defaultValue);
    }
    json value = m_json[tname];
    if(!value.is_boolean()) {
      SPDLOG_ERROR("Expected a bool for field {}.{}  got '{}'  ", rootPathString(), name, value.dump());
      throw std::runtime_error("Expected a bool in field.");
    }
    auto x = setChild(std::string(name), std::string(description), value.template get<bool>());
    return x->value();
  }

  std::any ConfigNodeJSON::initObject(const std::string_view &name,
                                      const std::string_view &description,
                                      const std::type_info &type,
                                      const std::string_view &defaultType)
  {
    ONDEBUG(SPDLOG_INFO("initObject {} for handle type '{}'  with default type '{}'  ", name, type.name(), defaultType));
    // Check if the 'name' is actually a path seperated by dots.

    std::string tname(name);
    json jvalue;
    std::shared_ptr<ConfigNode> node;
    if(m_json.find(tname) == m_json.end()) {
      // Try and use string as path if string has a dot in it.
      if(tname.find('.') != std::string::npos) {
        jvalue = tname;
      } else {
        SPDLOG_ERROR("Expected a object for {}.{} of type '{}' ", rootPathString(), name, typeName(type));
        throw std::runtime_error("Expected a object in field.");
      }
    } else {
      jvalue = m_json[tname];
    }
    if(jvalue.is_string()) {
      std::string path = jvalue.template get<std::string>();
      ONDEBUG(SPDLOG_INFO("Following path {} ", path));
      ConfigNode *aNode = nullptr;
      // We have to start from the parent, else we'll just find this entry again.
      if(m_parent != nullptr) {
        aNode = m_parent->followPath(path, type);
      } else {
        ONDEBUG(SPDLOG_INFO("No parent so starting path {} from root", path));
        aNode = followPath(path, type);
      }
      if(aNode == nullptr) {
        SPDLOG_ERROR("Failed to follow path '{}' from '{}'  Parent:{} ", path, rootPathString(), !m_parent);
        throw std::runtime_error("Failed to follow path ");
      }
      ONDEBUG(SPDLOG_INFO("Found node {} with type {} ", aNode->rootPathString(), aNode->value().type().name()));
      return aNode->value();
    }
    node = std::make_shared<ConfigNodeJSON>(*this,
                                            std::move(tname),
                                            std::string(description),
                                            std::any(),
                                            jvalue);
    m_children.emplace(node->name(), node);
    node->flagAsUsed("_type");
    std::string typeName = jvalue.value("_type", std::string(defaultType));
    ONDEBUG(SPDLOG_INFO("initObject '{}' for handle type '{}'  with default type '{}'  ", name, typeName, defaultType));
    Configuration config(*node);
    std::any val = m_factory->create(type, typeName, config);
    node->setValue(std::move(val));
    if(type != typeid(void)) {
      config.checkAllFieldsUsed();
    }
    ONDEBUG(SPDLOG_INFO("Created: {} ", node->value().type().name()));
    return node->value();
  }

  //! Get list of child nodes.
  std::vector<std::shared_ptr<ConfigNode>> ConfigNodeJSON::getChildNodes()
  {
    std::vector<std::shared_ptr<ConfigNode>> ret;
    for(auto it = m_json.begin(); it != m_json.end(); ++it) {
      ret.emplace_back(std::make_shared<ConfigNodeJSON>(*this,
                                                        std::string(it.key()),
                                                        std::string(),
                                                        std::any(),
                                                        *it));
    }
    return ret;
  }

  std::shared_ptr<ConfigNode> ConfigNodeJSON::setChild(std::string &&name, std::string &&description, std::any &&value)
  {
    json jval = m_json[name];
    std::shared_ptr<ConfigNode> node = std::make_shared<ConfigNodeJSON>(*this, std::move(name), std::move(description), std::move(value), jval);
    m_children.emplace(node->name(), node);
    return node;
  }

  void ConfigNodeJSON::checkAllFieldsUsed() const
  {
    bool ret = true;
    for(json::const_iterator it = m_json.begin(); it != m_json.end(); ++it) {
      std::string key = it.key();
      // Skip keys starting with -
      if(!key.empty() && key[0] == '-')
        continue;
      if(!isUsed(key)) {
        SPDLOG_ERROR("Field '{}' is not used in {} ", key, rootPathString());
        ret = false;
      }
    }
    if(!ret) {
      throw std::runtime_error("Unused fields in configuration.");
    }
  }

  bool ConfigNodeJSON::hasChild(const std::string_view &name)
  {
    if(ConfigNode::hasChild(name))
      return true;
    return m_json.find(name) != m_json.end();
  }

  std::shared_ptr<ConfigNode>
  ConfigNodeJSON::makeChildNode(std::string &&name, std::string &&description, std::any &&value)
  {
    std::string theName(name);
    json childJson = m_json[theName];
    if(childJson.is_null()) {
      SPDLOG_INFO("Failed to find child node '{}' in {} ", name, rootPathString());
    }
    return std::make_shared<ConfigNodeJSON>(*this, std::move(theName), std::move(description), std::move(value), childJson);
  }

}// namespace Ravl2