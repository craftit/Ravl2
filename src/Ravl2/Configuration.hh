//! @brief Configuration file handling
//! @author Charles Galambos
//! @date 06/03/2021
//! @details
//! This file contains the configuration file handling code. The source of the configuration isn't
//! delegated to another class which can be access via this interface without knowing the source.
//! The default source is a json file, but in the future it could be a database, python object etc.

#pragma once

#include <memory>
#include <any>
#include <set>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <sstream>
#include <utility>
#include <spdlog/spdlog.h>
#include "Ravl2/Assert.hh"
#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Get value from any container with boolean return value indicating success
  template <typename OutT>
  bool anyGet(const std::any &x, OutT &value)
  {
    try {
      value = std::any_cast<OutT>(x);
    } catch(std::bad_any_cast &) {
      return false;
    }
    return true;
  }

  //! Create an object from a string.
  template <typename DataT>
  inline DataT fromString(const std::string &text)
  {
    DataT val;
    std::istringstream is(text);
    is >> val;
    return val;
  }

  class Configuration;

  //! Factory for creating concrete types from a Configuration object

  class ConfigFactory : public std::enable_shared_from_this<ConfigFactory>
  {
  private:
    struct Entry {
      std::map<std::string, std::function<std::any(Configuration &)>, std::less<>> m_constructors;
    };
    std::unordered_map<std::type_index, Entry> m_map;

  public:
    ConfigFactory() = default;

    ConfigFactory(const ConfigFactory &) = delete;
    ConfigFactory(ConfigFactory &&) = delete;
    ConfigFactory &operator=(const ConfigFactory &) = delete;
    ConfigFactory &operator=(ConfigFactory &&) = delete;

    [[nodiscard]] std::any create(const std::type_info &type, std::string_view name, Configuration &config)
    {
      if(type == typeid(void)) {
        return {};
      }
      auto it = m_map.find(type);
      if(it == m_map.end()) {
        SPDLOG_ERROR("Type '{}' unknown when constructing '{}' ", type.name(), name);
        for(auto &a : m_map) {
          SPDLOG_ERROR("  Known type: {}", a.first.name());
        }
        throw std::runtime_error("type unknown");
      }
      auto it2 = it->second.m_constructors.find(name);
      if(it2 == it->second.m_constructors.end()) {
        SPDLOG_ERROR("No constructor for '{}' of type {} ", name, type.name());
        throw std::runtime_error("type unknown");
      }
      return it2->second(config);
    }

    //! Register a type with a constructor function
    bool add(const std::type_info &type, std::string_view name, const std::function<std::any(Configuration &)> &func)
    {
      auto &entry = m_map[std::type_index(type)].m_constructors[std::string(name)];
      if(entry) {
        SPDLOG_ERROR("Constructor '{}' already exists for type '{}'", name, type.name());
        throw std::runtime_error("type already exists");
      }
      entry = func;
      return true;
    }

    //! Register a named type.
    template <typename BaseDataT, typename DataT>
    bool registerNamedType(std::string_view name) noexcept
    {
      try {
        add(typeid(std::shared_ptr<BaseDataT>), name,
            [](Configuration &config) {
              return std::any(std::dynamic_pointer_cast<BaseDataT>(std::make_shared<DataT>(config)));
            });

      } catch(std::error_code &e) {
        SPDLOG_ERROR("Error registering type '{}' : {}", name, e.message());
        return false;
      }
      return true;
    }

    //! Register a named type.
    template <typename DataT>
    bool registerDirectType(std::string_view name) noexcept
    {
      try {
        add(typeid(DataT), name,
            [](Configuration &config) {
              return std::any(DataT(config));
            });
        add(typeid(DataT), "default",
            [](Configuration &config) {
              return std::any(DataT(config));
            });
      } catch(std::error_code &e) {
        SPDLOG_ERROR("Error registering type '{}' : {}", name, e.message());
        return false;
      }
      return true;
    }

    // Check a type is registered
    template <typename DataT,
              std::enable_if_t<!std::is_abstract<typename DataT::element_type>::value, bool> = true,
              std::enable_if_t<std::is_constructible<typename DataT::element_type, Configuration &>::value, bool> = true>
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(DataT))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return std::make_shared<typename DataT::element_type>(config); };
    }

    // Check a type is registered
    template <typename DataT,
              std::enable_if_t<!std::is_abstract<DataT>::value, bool> = true,
              std::enable_if_t<std::is_constructible<DataT, Configuration &>::value, bool> = true>
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(DataT))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return DataT(config); };
    }

    // Check a type is registered
    template <typename DataT,
              std::enable_if_t<std::is_abstract<DataT>::value, bool> = true>
    void checkRegistered(DataT &)
    {}

    // Check a type is registered
    template <typename DataT,
              std::enable_if_t<std::is_abstract<typename DataT::element_type>::value, bool> = true>
    void checkRegistered(DataT &)
    {}
  };

  ConfigFactory &defaultConfigFactory();

  //! @brief A constraint on a parameter, this checks the value is correct in some way.

  class Constraint
  {
  public:
    virtual ~Constraint() = default;

    Constraint() = default;

    //! Disable copy constructor
    Constraint(const Constraint &) = delete;
    Constraint(Constraint &&) = delete;
    Constraint &operator=(const Constraint &other) = delete;
    Constraint &operator=(Constraint &&other) = delete;

    //! @brief Check to see if the value is valid
    //! @param x The value to check
    //! @return true if the value is valid
    [[nodiscard]]
    virtual bool apply(const std::any &x) const = 0;
  };

  //! @brief Numeric parameter entry
  template <typename ValueT>
  class ConstraintNumeric : public Constraint
  {
  public:
    ConstraintNumeric(ValueT min, ValueT max)
        : m_min(min),
          m_max(max)
    {
    }

    //! @brief Check to see if the value is valid
    [[nodiscard]] bool apply(const std::any &x) const override
    {
      assert(x.type() == typeid(ValueT));
      auto v = std::any_cast<ValueT>(x);
      if(v < m_min) {
        SPDLOG_ERROR("Value {} below minimum value {} ", v, m_min);
        throw std::runtime_error("Illegal configuration parameter");
      }
      if(v > m_max) {
        SPDLOG_ERROR("Value {} above maximum value {} ", v, m_max);
        throw std::runtime_error("Illegal configuration parameter");
      }
      return true;
    }

  protected:
    ValueT m_min = 0;
    ValueT m_max = 0;
  };

  //! @brief Node in a configuration tree

  class ConfigNode : public std::enable_shared_from_this<ConfigNode>
  {
  public:
    //! Create a new empty node
    ConfigNode();

    //! Create a new node with a source file
    ConfigNode(const std::string_view &filename);

    //! Make destructor virtual
    virtual ~ConfigNode() = default;

    ConfigNode(const ConfigNode &) = delete;
    ConfigNode(ConfigNode &&) = delete;
    ConfigNode &operator=(const ConfigNode &) = delete;
    ConfigNode &operator=(ConfigNode &&) = delete;

    ConfigNode(ConfigNode &parent, const std::string_view &name, const std::string_view &description, const std::any &value)
        : m_factory(parent.factory().shared_from_this()),
          m_name(name),
          m_description(description),
          m_value(value),
          m_parent(&parent)
    {}

    ConfigNode(ConfigNode &parent, std::string &&name, std::string &&description, std::any &&value)
        : m_factory(parent.factory().shared_from_this()),
          m_name(std::move(name)),
          m_description(std::move(description)),
          m_value(std::move(value)),
          m_parent(&parent)
    {}

    //! Access name of node.
    [[nodiscard]] const std::string &name() const
    {
      return m_name;
    }

    //! Access the source file name.
    [[nodiscard]] const std::string &filename() const
    {
      return m_filename;
    }

    //! Access the parent node.
    [[nodiscard]] ConfigNode *parent() const
    {
      return m_parent;
    }

    //! Access the parent node.
    [[nodiscard]] const ConfigNode &rootNode() const
    {
      const ConfigNode *node = this;
      while(node->m_parent != nullptr) {
        node = node->m_parent;
      }
      return *node;
    }

    //! Access the parent node.
    [[nodiscard]] ConfigNode &rootNode()
    {
      ConfigNode *node = this;
      while(node->m_parent != nullptr) {
        node = node->m_parent;
      }
      return *node;
    }

    //! Get path to this node.
    [[nodiscard]] std::string path() const;

    //! Access description of node
    [[nodiscard]] const std::string &description() const
    {
      return m_description;
    }

    //! Access value of node.
    [[nodiscard]] std::any &value()
    {
      return m_value;
    }

    //! Access factory being used.
    [[nodiscard]] ConfigFactory &factory()
    {
      assert(m_factory);
      return *m_factory;
    }

    //! Initialise a number field
    [[nodiscard]] virtual std::any initNumber(const std::string_view &name, const std::string_view &description, int defaultValue, int min, int max);

    //! Initialise a number field
    [[nodiscard]] virtual std::any initNumber(const std::string_view &name, const std::string_view &description, unsigned defaultValue, unsigned min, unsigned max);

    //! Initialise a number field
    [[nodiscard]] virtual std::any initNumber(const std::string_view &name, const std::string_view &description, size_t defaultValue, size_t min, size_t max);

    //! Initialise a number field
    [[nodiscard]] virtual std::any initNumber(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max);

    //! Initialise a number field
    [[nodiscard]] virtual std::any initNumber(const std::string_view &name, const std::string_view &description, double defaultValue, double min, double max);
    
    //! Initialise a vector field
    [[nodiscard]] virtual std::any initVector(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max,size_t size);
    
    //! Initialise a string field
    [[nodiscard]] virtual std::any initString(const std::string_view &name, const std::string_view &description, const std::string_view &defaultValue);

    //! Initialise a string field
    [[nodiscard]] virtual std::any initBool(const std::string_view &name, const std::string_view &description, bool defaultValue);

    //! Is the value of a field defined in the configuration file?
    [[nodiscard]] virtual bool isDefined(const std::string_view &name) const
    {
      (void)name;
      return false;
    }

    //! Get member name in string form
    [[nodiscard]] virtual std::string asString(const std::string_view &name) const
    {
      (void)name;
      return "";
    }

    //! Initialise a string field
    template <typename DataT>
    std::any init(
      const std::string_view &name,
      const std::string_view &description,
      const DataT &defaultValue)
    {
      std::shared_ptr<ConfigNode> node;
      if(isDefined(name)) {
        node = setChild(std::string(name), std::string(description), fromString<DataT>(asString(name)));
      } else {
        node = setChild(std::string(name), std::string(description), defaultValue);
      }
      return node->value();
    }

    //! Initialise an object field
    virtual std::any initObject(const std::string_view &name,
                                const std::string_view &description,
                                const std::type_info &type,
                                const std::string_view &defaultType);

    //! Get a list of child nodes.
    virtual std::vector<std::shared_ptr<ConfigNode>> getChildNodes()
    {
      return {};
    }

    //! Create a child node.
    virtual std::shared_ptr<ConfigNode> makeChildNode(std::string &&name, std::string &&description,
                                                      std::any &&value)
    {
      return std::make_shared<ConfigNode>(*this, name, description, value);
    }

    //! Initialise an array field
    //! \param name Name of container object
    //! \param description Description of container object
    //! \param type Type of container object
    //! \param defaultType Default type to use for contained objects
    template <typename DataT>
    std::any initObjectArray(const std::string_view &name,
                             const std::string_view &description,
                             const std::string_view &defaultType)
    {
      std::shared_ptr<ConfigNode> node = makeChildNode(std::string(name), std::string(description), std::any());
      m_children.emplace(node->name(), node);
      auto nodes = node->getChildNodes();
      //SPDLOG_INFO("initObjectArray: {}",nodes.size());
      std::vector<DataT> ret;
      ret.reserve(nodes.size());
      for(auto const &child : nodes) {
        std::any obj = node->initObject(child->name(), description, typeid(DataT), defaultType);
        ret.push_back(std::any_cast<DataT>(obj));
      }
      node->setValue(ret);
      return ret;
    }

    //! Do we have a child in the config, even if not created yet?
    [[nodiscard]] virtual bool hasChild(const std::string_view &name);

    //! Flag field as used.
    void flagAsUsed(const std::string_view &field)
    {
      m_usedFields.emplace(field);
    }

    //! Query if a field as been used
    //! It seems we can't query with a std::string_view
    [[nodiscard]] bool isUsed(const std::string &field) const
    {
      return m_usedFields.find(field) != m_usedFields.end();
    }

    //! Get value.
    [[nodiscard]] std::any getValue(const std::string_view &name, const std::type_info &type);

    //! Set child value.
    virtual std::shared_ptr<ConfigNode> setChild(std::string &&name, std::string &&description, std::any &&value);

    //! Get path from root as string
    [[nodiscard]] std::string rootPathString() const;

    //! Follow path to a node.
    //! If the node is not found a nullptr will be returned.
    [[nodiscard]] ConfigNode *followPath(std::string_view path, const std::type_info &type);

    //! Follow path to a node.
    //! If the node is not found a nullptr will be returned.
    [[nodiscard]] ConfigNode *followPath(const std::vector<std::string_view> &path, const std::type_info &type);

    //! Get child node.
    [[nodiscard]] ConfigNode *child(std::string_view name, std::string_view description);

    //! Set value held in node.
    void setValue(std::any &&v)
    {
      m_value = std::move(v);
    }

    //! Check all subfields are used.
    virtual void checkAllFieldsUsed() const;

  protected:
    std::shared_ptr<ConfigFactory> m_factory;
    std::string m_filename;
    std::string m_name;
    std::string m_description;
    std::any m_value;
    std::set<std::string, std::less<>> m_usedFields;
    std::map<std::string_view, std::shared_ptr<ConfigNode>> m_children;
    ConfigNode *m_parent = nullptr;
  };

  //! Configuration handle

  class Configuration
  {
  public:
    //! Load configuration from a file.
    explicit Configuration(const std::string_view &filename);

    //! Load configuration from a JSON string
    static Configuration fromJSONString(const std::string_view &data);

    //! Create from a node.
    explicit Configuration(ConfigNode &node)
        : m_node(node.shared_from_this())
    {}

    //! Create from a node.
    explicit Configuration(std::shared_ptr<ConfigNode> &&node)
        : m_node(node)
    {}

    //! Path to this node
    [[nodiscard]] std::string path() const
    {
      assert(m_node);
      return m_node->path();
    }

    //! Name of this node
    [[nodiscard]] const std::string &name() const
    {
      assert(m_node);
      return m_node->name();
    }

    //! Name of this node
    [[nodiscard]] const std::string &filename() const
    {
      assert(m_node);
      return m_node->rootNode().filename();
    }

    template <typename DataT,typename ParamT = DataT>
     requires std::is_convertible_v<ParamT,DataT>
    [[nodiscard]] DataT getNumber(const std::string_view &name, const std::string_view &description, DataT defaultValue, ParamT min, ParamT max)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(DataT));
      if(!value.has_value()) {
        value = m_node->initNumber(name, description, DataT(defaultValue), DataT(min), DataT(max));
      }
      return std::any_cast<DataT>(value);
    }
    
    template <typename DataT>
    [[nodiscard]] std::vector<DataT> getNumericVector(const std::string_view &name, const std::string_view &description, DataT defaultValue, DataT min, DataT max,size_t size)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::vector<DataT>));
      if(!value.has_value()) {
        value = m_node->initVector(name, description, float(defaultValue), float(min), float(max),size);
      }
      return std::any_cast<std::vector<DataT> >(value);
    }
    
    //! Get a point from the configuration file.
    template <typename RealT,IndexSizeT N,typename ParamT = RealT>
     requires std::is_convertible_v<ParamT,RealT>
    [[nodiscard]] Point<RealT,N> getPoint(const std::string_view &name, const std::string_view &description, RealT defaultValue, ParamT min, ParamT max)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::vector<float>));
      if(!value.has_value()) {
        value = m_node->initVector(name, description, float(defaultValue),float(min), float(max),N);
      }
      auto vec = std::any_cast<std::vector<float>>(value);
      Point<RealT,N> ret;
      if(vec.size() != N) {
        SPDLOG_ERROR("Expected {} elements in point, got {} ", N, vec.size());
        throw std::runtime_error("Wrong number of elements in point");
      }
      for(int i = 0; i < int(N); i++) {
        ret[i] = RealT(vec[size_t(i)]);
      }
      return ret;
    }
    
    //! This reads a matrix from the configuration file.
    //! The data is stored as a vector in row major order.
    template <typename RealT,IndexSizeT N,IndexSizeT M,typename ParamT = RealT>
      requires std::is_convertible_v<ParamT,RealT>
    [[nodiscard]] Matrix<RealT,N,M> getMatrix(const std::string_view &name, const std::string_view &description, RealT defaultValue, RealT min, RealT max)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::vector<float>));
      if(!value.has_value()) {
        value = m_node->initVector(name, description, float(defaultValue),float(min), float(max),N * M);
      }
      auto vec = std::any_cast<std::vector<float>>(value);
      Matrix<RealT,N,M> ret;
      if(vec.size() != N * M) {
        SPDLOG_ERROR("Expected {} elements in matrix, got {} ", N * M, vec.size());
        throw std::runtime_error("Wrong number of elements in matrix");
      }
      for(size_t i = 0; i < N; i++) {
        for(size_t j = 0; j < M; j++) {
          ret(int(i),int(j)) = RealT(vec[i * M + j]);
        }
      }
      return ret;
    }
    
    [[nodiscard]] std::string getString(const std::string_view &name, const std::string_view &description, const std::string_view &defaultValue)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::string));
      if(!value.has_value()) {
        value = m_node->initString(name, description, defaultValue);
      }
      return std::any_cast<std::string>(value);
    }

    [[nodiscard]] bool getBool(const std::string_view &name, const std::string_view &description, bool defaultValue)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(bool));
      if(!value.has_value()) {
        value = m_node->initBool(name, description, defaultValue);
      }
      return std::any_cast<bool>(value);
    }

    template <typename T>
    [[nodiscard]] T get(const std::string_view &name, const std::string_view &description, const T &defaultValue)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(T));
      if(!value.has_value()) {
        value = m_node->init(name, description, defaultValue);
      }
      return std::any_cast<T>(value);
    }

    //! @brief Compulsory component

    template <typename DataT>
    void useComponent(const std::string_view &name, const std::string_view &description, DataT &value, const std::string_view &defaultType = "default")
    {
      assert(m_node);
      m_node->factory().checkRegistered<DataT>(value);
      std::any avalue = m_node->getValue(name, typeid(DataT));
      if(!avalue.has_value()) {
        avalue = m_node->initObject(name, description, typeid(DataT), defaultType);
      }
      //SPDLOG_INFO("Created type: {} for target {} ",avalue.type().name(),typeid(DataT).name());
      if(!anyGet(avalue, value)) {
        SPDLOG_ERROR("Type mismatch for field '{}' expected '{}' got '{}' ", name, typeName(typeid(DataT)), typeName(avalue.type()));
        throw std::runtime_error("Type mismatch");
      }
    }

    //! @brief Optional component

    template <typename DataT>
    bool optComponent(const std::string_view &name, const std::string_view &description, DataT &value, const std::string_view &defaultType = "default")
    {
      if(!hasChild(name))
        return false;
      useComponent(name, description, value, defaultType);
      return true;
    }

    //! @brief Get an array of objects.

    template <class T>
    bool useGroup(const std::string_view &name, const std::string_view &description, std::vector<T> &objects, const std::string_view &defaultType = "default")
    {
      assert(m_node);
      std::any avalue = m_node->getValue(name, typeid(std::vector<T>));
      if(!avalue.has_value()) {
        avalue = m_node->initObjectArray<T>(name, description, defaultType);
      }
      objects = std::any_cast<std::vector<T>>(avalue);
      return true;
    }

    //! @brief Return an array of objects.
    template <class T>
    std::vector<T> getVector(const std::string_view &name, const std::string_view &description, const std::string_view &defaultType = "default")
    {
      assert(m_node);
      std::any avalue = m_node->getValue(name, typeid(std::vector<T>));
      if(!avalue.has_value()) {
        avalue = m_node->initObjectArray<T>(name, description, defaultType);
      }
      return std::any_cast<std::vector<T>>(avalue);
    }

    //! Do we have a child node?
    [[nodiscard]] bool hasChild(const std::string_view &name) const
    {
      assert(m_node);
      return m_node->hasChild(name);
    }

    //! @brief Get a named types less child node.
    //! The child node must exist, but may be empty. The node created
    //! is untyped and it's there is no check if all its contents are used.
    //! @param name - Name of the child node
    //! @param description - Description of the child node
    //! @return config
    [[nodiscard]] Configuration child(std::string_view name, std::string_view description) const
    {
      assert(m_node);
      auto *childNode = m_node->child(name, description);
      if(!childNode) {
        SPDLOG_ERROR("No child node '{}' found at {} ", name, m_node->rootPathString());
        throw std::runtime_error("Named child not found");
      }
      return Configuration {*childNode};
    }

    //! Check all subfields are used.
    //! This will throw an exception if any fields are unused.
    //! @return true if all fields are used.
    void checkAllFieldsUsed() const
    {
      m_node->checkAllFieldsUsed();
    }

    //! Check if this is a valid configuration.
    [[nodiscard]] bool isValid() const
    {
      return m_node != nullptr;
    }
    
  protected:
    std::shared_ptr<ConfigNode> m_node;
  };

}// namespace Ravl2