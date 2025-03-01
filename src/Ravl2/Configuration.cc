//
// Created by Charles Galambos on 06/03/2021.
//

#include "Ravl2/Configuration.hh"
#include "Ravl2/StringUtils.hh"
#include "Ravl2/IO/TypeConverter.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  ConfigFactory &defaultConfigFactory() noexcept
  {
    static std::shared_ptr<ConfigFactory> factory = std::make_shared<ConfigFactory>();
    return *factory;
  }

  //----------------------------------------------------------

  ConfigNode::ConfigNode()
      : m_factory(defaultConfigFactory().shared_from_this()),
        m_name("root")
  {}
  
  ConfigNode::ConfigNode(const std::string_view &filename)
      : m_factory(defaultConfigFactory().shared_from_this()),
        m_filename(filename),
        m_name("root")
  {}

  const ConfigNode &ConfigNode::rootNode() const
  {
    const ConfigNode *node = this;
    while(node->m_parent != nullptr) {
      node = node->m_parent;
    }
    return *node;
  }

  ConfigNode &ConfigNode::rootNode()
  {
    ConfigNode *node = this;
    while(node->m_parent != nullptr) {
      node = node->m_parent;
    }
    return *node;
  }

  //! Get path to this node.
  std::string ConfigNode::path() const
  {
    if(!m_parent) {
      return name();
    }
    return fmt::format("{}.{}", m_parent->path(), m_name);
  }

  template <typename DataT>
  static void checkRange(const std::any &x, DataT min, DataT max)
  {
    assert(x.type() == typeid(DataT));
    auto v = std::any_cast<DataT>(x);
    if(v < min) {
      SPDLOG_ERROR("Value {} below minimum value {} ", v, min);
      throw std::runtime_error("Illegal configuration parameter");
    }
    if(v > max) {
      SPDLOG_ERROR("Value {} above maximum value {} ", v, max);
      throw std::runtime_error("Illegal configuration parameter");
    }
  }

  std::any ConfigNode::initNumber(
    const std::string_view &name, const std::string_view &description, int defaultValue, int min, int max)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    checkRange(x->value(), min, max);
    return x->value();
  }

  std::any ConfigNode::initNumber(
    const std::string_view &name, const std::string_view &description, unsigned defaultValue, unsigned min, unsigned max)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    checkRange(x->value(), min, max);
    return x->value();
  }

  //! Initialise a number field
  std::any ConfigNode::initNumber(const std::string_view &name, const std::string_view &description, size_t defaultValue, size_t min, size_t max)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    checkRange(x->value(), min, max);
    return x->value();
  }

  std::any ConfigNode::initNumber(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    checkRange(x->value(), min, max);
    return x->value();
  }

  std::any ConfigNode::initNumber(const std::string_view &name, const std::string_view &description, double defaultValue, double min, double max)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    checkRange(x->value(), min, max);
    return x->value();
  }
  
  //! Initialise a vector field
  [[nodiscard]] std::any ConfigNode::initVector(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max,size_t size)
  {
    std::vector<float> vec(size,defaultValue);
    auto x = setChild(std::string(name), std::string(description),vec);
    for(auto &v : std::any_cast<std::vector<float>>(x->value())) {
      checkRange(v, min, max);
    }
    return x->value();
  }
  
  
  std::any ConfigNode::initObject(const std::string_view &name,
                                  const std::string_view &description,
                                  const std::type_info &type,
                                  const std::string_view &defaultType)
  {
    std::shared_ptr<ConfigNode> node = std::make_shared<ConfigNode>(*this, name, description, std::any());
    m_children.emplace(node->name(), node);
    Configuration config(*node);
    node->setValue(m_factory->create(type, defaultType, config));
    config.checkAllFieldsUsed();
    return node->value();
  }

  //! Initialise a string field
  std::any ConfigNode::initString(
    const std::string_view &name,
    const std::string_view &description,
    const std::string_view &defaultValue)
  {
    ONDEBUG(SPDLOG_INFO("initString for {} default is '{}' ", name, defaultValue));
    auto x = setChild(std::string(name), std::string(description), std::string(defaultValue));
    return x->value();
  }

  std::any ConfigNode::initBool(const std::string_view &name, const std::string_view &description, bool defaultValue)
  {
    auto x = setChild(std::string(name), std::string(description), defaultValue);
    return x->value();
  }

  std::any ConfigNode::getValue(const std::string_view &name, [[maybe_unused]] const std::type_info &type)
  {
    flagAsUsed(name);
    auto x = m_children.find(name);
    if(x == m_children.end()) {
      return {};
    }
    assert(x->second->value().type() == type);
    return x->second->value();
  }

  std::shared_ptr<ConfigNode> ConfigNode::setChild(std::string &&name, std::string &&description, std::any &&value)
  {
    std::shared_ptr<ConfigNode> node = std::make_shared<ConfigNode>(*this, std::move(name), std::move(description), std::move(value));
    m_children.emplace(node->name(), node);
    return node;
  }

  std::string ConfigNode::rootPathString() const
  {
    std::string ret;
    ret.reserve(256);
    auto at = this;
    while(at->m_parent != nullptr) {
      ret = at->m_name + '.' + ret;
      at = at->m_parent;
    }
    return ret;
  }

  void ConfigNode::checkAllFieldsUsed() const
  {
  }

  ConfigNode *ConfigNode::followPath(std::string_view pathstr, const std::type_info &type)
  {
    std::vector<std::string_view> const thePath = split(pathstr, '.');
    return followPath(thePath, type);
  }

  //! Follow path to a node.
  //! If the node is not found a nullptr will be returned.
  [[nodiscard]] ConfigNode *ConfigNode::followPath(const std::vector<std::string_view> &path, const std::type_info &type)
  {
    ConfigNode *startAt = this;
    while(startAt != nullptr) {
      ConfigNode *at = startAt;
      for(auto pit = path.begin(); pit != path.end(); ++pit) {
        auto &x = *pit;
        auto it = at->m_children.find(x);
        if(it == at->m_children.end()) {
          if(at->hasChild(x)) {
            if(pit != path.end() - 1) {
              SPDLOG_WARN("Can't create intermediate objects in path. at {} for {} ", at->rootPathString(), x);
            } else {
              //SPDLOG_INFO("Initialising object at {} for {} of type {} ",at->rootPathString(),x,type.name());
              at->initObject(x, "Unknown", type, "default");
              it = at->m_children.find(x);
              if(it == at->m_children.end()) {
                SPDLOG_ERROR("Failed to create object at {} {}  ", at->rootPathString(), x);
                throw std::runtime_error("Failed to create object.");
              }
              return it->second.get();
            }
          }
          at = nullptr;
          break;
        }
        at = it->second.get();
      }
      if(at != nullptr)
        return at;
      // Failed to follow path from current node, try its parent
      startAt = startAt->m_parent;
    }
    return nullptr;
  }

  bool ConfigNode::hasChild(const std::string_view &name)
  {
    return m_children.find(name) != m_children.end();
  }

  //! Get child node.
  ConfigNode *ConfigNode::child(std::string_view name, std::string_view description)
  {
    auto it = m_children.find(name);
    if(it != m_children.end()) {
      return it->second.get();
    }
    if(!hasChild(name)) {
      return nullptr;
    }
    // Create a dummy entry
    initObject(name, description, typeid(void), "default");

    it = m_children.find(name);
    if(it == m_children.end()) {
      SPDLOG_ERROR("Failed to create child group {} ", name);
      return nullptr;
    }
    flagAsUsed(name);
    return it->second.get();
  }

  bool ConfigNode::setValue(std::any &&v)
  {
    if(m_fieldType == nullptr || *m_fieldType == typeid(void)) {
      m_fieldType = &m_value.type();
      m_value = std::move(v);
    } else {
      //SPDLOG_INFO("Setting value of {}, has update:{}  type:{}  from:{} ", name(), hasUpdate(), typeName(*m_fieldType), typeName(v.type()));
      if(*m_fieldType != v.type()) {
        auto optValue = typeConverterMap().convert(*m_fieldType, v);
        if(!optValue) {
          SPDLOG_ERROR("Failed to convert {} to {} ", typeName(v.type()), typeName(*m_fieldType));
          return false;
        }
        assert(optValue->type() == *m_fieldType);
        m_value = std::move(*optValue);
      } else {
        m_value = std::move(v);
      }
    }
    if(mUpdate) {
      return mUpdate(m_value);
    }
    return true;
  }

}// namespace Ravl2
