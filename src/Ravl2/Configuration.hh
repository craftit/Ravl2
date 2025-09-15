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
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{


  class Configuration;

  //! Factory for creating concrete types from a Configuration object

  class ConfigFactory : public std::enable_shared_from_this<ConfigFactory>
  {
  private:
    struct Entry {
      std::map<std::string, std::function<std::any(Configuration &)>, std::less<>> m_constructors;
    };
    std::unordered_map<std::type_index, Entry> m_map;
    std::unordered_map<std::string,  std::function<std::any(Configuration &)>> m_name2factory;

  public:
    ConfigFactory() = default;

    ConfigFactory(const ConfigFactory &) = delete;
    ConfigFactory(ConfigFactory &&) = delete;
    ConfigFactory &operator=(const ConfigFactory &) = delete;
    ConfigFactory &operator=(ConfigFactory &&) = delete;

    [[nodiscard]] std::any create(const std::type_info &type, std::string_view name, Configuration &config);

    //! Register a type with a constructor function
    bool add(const std::type_info &type, std::string_view name, const std::function<std::any(Configuration &)> &func);

    //! Register a named type.
    template <typename BaseDataT, typename DataT>
    bool registerNamedType(std::string_view name) noexcept
    {
      try {
        add(typeid(std::shared_ptr<BaseDataT>), name,
            [](Configuration &config) {
              return std::any(std::make_shared<DataT>(config));
            });
        registerConversion([](const std::shared_ptr<DataT> &derived) -> std::shared_ptr<BaseDataT> {
          return std::dynamic_pointer_cast<BaseDataT>(derived);
        } , 1.0f);
      } catch(std::error_code &e) {
        SPDLOG_ERROR("Error registering type '{}' : {}", name, e.message());
        return false;
      }
      return true;
    }

    //! Register a named type.
    template <typename DataT>
     requires (std::is_constructible<DataT, Configuration &>::value && std::is_copy_constructible<DataT>::value)
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

    template <typename DataT>
       requires (std::is_constructible<DataT, Configuration &>::value && !std::is_copy_constructible<DataT>::value)
    bool registerDirectType(std::string_view name) noexcept
    {
      try {
        add(typeid(std::shared_ptr<DataT>), name,
            [](Configuration &config) {
              return std::any(std::make_shared<DataT>(config));
            });
        add(typeid(std::shared_ptr<DataT>), "default",
            [](Configuration &config) {
              return std::any(std::make_shared<DataT>(config));
            });
      } catch(std::error_code &e) {
        SPDLOG_ERROR("Error registering type '{}' : {}", name, e.message());
        return false;
      }
      return true;
    }

    // Check a type is registered
    template <typename DataT>
     requires (!std::is_copy_constructible<DataT>::value &&
       std::is_constructible<DataT, Configuration &>::value &&
       !std::is_abstract<DataT>::value)
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(std::shared_ptr<DataT>))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return std::make_shared<DataT>(config); };
    }

    // Check a type is registered
    template <typename DataT>
     requires (std::is_constructible<typename DataT::element_type, Configuration &>::value &&
       !std::is_abstract<typename DataT::element_type>::value)
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(DataT))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return std::make_shared<typename DataT::element_type>(config); };
    }

    // Check a type is registered
    template <typename DataT>
      requires (std::is_constructible<DataT, Configuration &>::value &&
                std::is_copy_constructible<DataT>::value &&
                !std::is_abstract<DataT>::value )
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(DataT))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return std::any(DataT(config)); };
    }

    template <typename DataT,
          std::enable_if_t<!std::is_abstract<DataT>::value, bool> = true,
          std::enable_if_t<std::is_constructible<DataT, Configuration &>::value, bool> = true>
    requires (std::is_constructible<typename DataT::element_type, Configuration &>::value && !std::is_copy_constructible<DataT>::value)
    void checkRegistered(DataT &)
    {
      auto &entry = m_map[std::type_index(typeid(DataT))].m_constructors["default"];
      if(entry) return;
      entry = [](Configuration &config) { return std::make_shared<DataT>(config); };
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

  ConfigFactory &defaultConfigFactory() noexcept;

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
    explicit ConfigNode(const std::string_view &filename);

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
    {
      m_fieldType = &m_value.type();
    }

    ConfigNode(ConfigNode &parent, std::string &&name, std::string &&description, std::any &&value)
        : m_factory(parent.factory().shared_from_this()),
          m_name(std::move(name)),
          m_description(std::move(description)),
          m_value(std::move(value)),
          m_parent(&parent)
    {
      m_fieldType = &m_value.type();
    }

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
    [[nodiscard]] const ConfigNode &rootNode() const;

    //! Access the parent node.
    [[nodiscard]] ConfigNode &rootNode();

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

    //! Get existing value from node.
    template <typename T>
    [[nodiscard]] T get()
    {
      if(!m_value.has_value()) {
        SPDLOG_ERROR("No value found for field '{}' ", name());
        throw std::runtime_error("No value found");
      }
      return std::any_cast<T>(m_value);
    }

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
    bool setValue(std::any &&v);

    //! Set value held in node.
    template <typename DataT>
    bool setValue(DataT v)
    {
      return setValue(std::any(v));
    }

    //! Check all subfields are used.
    virtual void checkAllFieldsUsed() const;

    //! Set update function
    void setUpdate(std::string_view const &name, std::function<bool(std::any &update)> update)
    {
      auto child = m_children.find(name);
      if(child == m_children.end()) {
        SPDLOG_ERROR("No child '{}' found for update ", name);
        throw std::runtime_error("No child found for update");
      }
      if(child->second->mUpdate) {
        SPDLOG_ERROR("Update function already set for {} ", path());
        throw std::runtime_error("Update function already set");
      }
      child->second->mUpdate = std::move(update);
    }

    //! Test if update provided
    [[nodiscard]] bool hasUpdate() const
    {
      return mUpdate != nullptr;
    }

    //! Access children
    std::map<std::string_view, std::shared_ptr<ConfigNode>> &children()
    { return m_children; };

    //! Access node type
    [[nodiscard]] const std::type_info &fieldType() const
    { if(m_fieldType != nullptr) return *m_fieldType; return typeid(void); }

  protected:
    std::shared_ptr<ConfigFactory> m_factory;
    std::string m_filename;
    std::string m_name;
    std::string m_description;
    std::any m_value;
    std::type_info const *m_fieldType = nullptr;
    std::set<std::string, std::less<>> m_usedFields {};
    std::map<std::string_view, std::shared_ptr<ConfigNode>> m_children {};
    ConfigNode *m_parent = nullptr;
    std::function<bool(std::any &update)> mUpdate;
  };

  //! Configuration handle

  class Configuration
  {
  public:
    //! Default constructor
    Configuration() = default;

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
    //! @return Path as a string with '.' between elements.
    [[nodiscard]] std::string path() const
    {
      assert(m_node);
      return m_node->path();
    }

    //! Name of this node
    //! @return Name of this node
    [[nodiscard]] const std::string &name() const
    {
      assert(m_node);
      return m_node->name();
    }

    //! Filename this node was loaded from
    //! @return Filename
    [[nodiscard]] const std::string &filename() const
    {
      assert(m_node);
      return m_node->rootNode().filename();
    }

    //! Get number
    template <typename DataT,typename ParamT = DataT>
     requires std::is_convertible_v<ParamT,DataT>
    [[nodiscard]] DataT getNumber(const std::string_view &name, const std::string_view &description, DataT defaultValue, ParamT min, ParamT max,std::function<bool(DataT)> update = {})
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(DataT));
      if(!value.has_value()) {
        value = m_node->initNumber(name, description, DataT(defaultValue), DataT(min), DataT(max));
      }
      if(update) {
        m_node->setUpdate(name, [update](std::any &newValue) {
          return update(std::any_cast<DataT>(newValue));
        });
      }
      return std::any_cast<DataT>(value);
    }

    //! Get an integer from the configuration file.
    [[nodiscard]] int getInt(const std::string_view &name, const std::string_view &description, int defaultValue, int min, int max,std::function<bool(int)> update = {})
    { return getNumber<int>(name, description, defaultValue, min, max,update); }

    //! Get an unsigned integer from the configuration file.
    [[nodiscard]] unsigned getUnsigned(const std::string_view &name, const std::string_view &description, unsigned defaultValue, unsigned min, unsigned max, std::function<bool(unsigned)> update = {})
    { return getNumber<unsigned>(name, description, defaultValue, min, max,update); }

    //! Get a float from the configuration file.
    [[nodiscard]] float getFloat(const std::string_view &name, const std::string_view &description, float defaultValue, float min, float max, std::function<bool(float)> update = {})
    { return getNumber<float>(name, description, defaultValue, min, max,update); }

    //! Get a float from the configuration file.
    template<typename RealT>
    [[nodiscard]] RealT getReal(const std::string_view &name, const std::string_view &description, RealT defaultValue, RealT min, RealT max, std::function<bool(float)> update = {})
    { return getNumber<RealT>(name, description, defaultValue, min, max,update); }

    template <typename DataT>
    [[nodiscard]] std::vector<DataT> getNumericVector(const std::string_view &name, const std::string_view &description, DataT defaultValue, DataT min, DataT max,size_t size)
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::vector<DataT>));
      if(!value.has_value()) {
        value = m_node->initVector(name, description, static_cast<float>(defaultValue), static_cast<float>(min), static_cast<float>(max),size);
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
        value = m_node->initVector(name, description, static_cast<float>(defaultValue), static_cast<float>(min), static_cast<float>(max),N);
      }
      auto vec = std::any_cast<std::vector<float>>(value);
      Point<RealT,N> ret;
      if(vec.size() != N) {
        SPDLOG_ERROR("Expected {} elements in point, got {} ", N, vec.size());
        throw std::runtime_error("Wrong number of elements in point");
      }
      for(int i = 0; i < N; i++) {
        ret[i] = RealT(vec[static_cast<size_t>(i)]);
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
        value = m_node->initVector(name, description, static_cast<float>(defaultValue), static_cast<float>(min), static_cast<float>(max),N * M);
      }
      auto vec = std::any_cast<std::vector<float>>(value);
      Matrix<RealT,N,M> ret;
      if(vec.size() != N * M) {
        SPDLOG_ERROR("Expected {} elements in matrix, got {} ", N * M, vec.size());
        throw std::runtime_error("Wrong number of elements in matrix");
      }
      for(size_t i = 0; i < N; i++) {
        for(size_t j = 0; j < M; j++) {
          ret(static_cast<int>(i), static_cast<int>(j)) = RealT(vec[i * M + j]);
        }
      }
      return ret;
    }

    [[nodiscard]] std::string getString(const std::string_view &name, const std::string_view &description, const std::string_view &defaultValue,std::function<bool(const std::string &)> update = {})
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(std::string));
      if(!value.has_value()) {
        value = m_node->initString(name, description, defaultValue);
      }
      if(update) {
        m_node->setUpdate(name, [update](std::any &newValue) {
          return update(std::any_cast<std::string>(newValue));
        });
      }
      return std::any_cast<std::string>(value);
    }

    [[nodiscard]] bool getBool(const std::string_view &name, const std::string_view &description, bool defaultValue,std::function<bool(bool)> update = {})
    {
      assert(m_node);
      std::any value = m_node->getValue(name, typeid(bool));
      if(!value.has_value()) {
        value = m_node->initBool(name, description, defaultValue);
      }
      if(update) {
        m_node->setUpdate(name, [update](std::any &newValue) {
          return update(std::any_cast<bool>(newValue));
        });
      }

      return std::any_cast<bool>(value);
    }


    //! Helper to get a time duration as a std::chrono type
    //! If duration is given as a plain number, it is assumed to be in seconds.
    //! Otherwise it is a string with postfix of the same form as std::chrono literals.
    //! @param name Name of the field
    //! @param description Description of the field
    //! @param defaultValue Default value
    //! @param minV Minimum value
    //! @param maxV Maximum value
    //! @param update Update function
    //! @return Duration
    template<typename Rep, typename Period = std::ratio<1>>
    [[nodiscard]] std::chrono::duration<Rep,Period> getDuration(
        const std::string_view &name,
        const std::string_view &description,
        std::chrono::duration<Rep,Period> defaultValue,
        std::chrono::duration<Rep,Period> minV = std::chrono::duration<Rep,Period>(std::numeric_limits<Rep>::lowest()),
        std::chrono::duration<Rep,Period> maxV = std::chrono::duration<Rep,Period>(std::numeric_limits<Rep>::max()),
          std::function<bool(std::chrono::duration<Rep,Period>)> update = {})
    {
      using DurationT = std::chrono::duration<Rep,Period>;
      if(defaultValue < minV || defaultValue > maxV) {
        SPDLOG_ERROR("Default duration {} out of range {} - {}",defaultValue,minV,maxV);
        throw std::runtime_error("Default duration out of range");
      }
      // Check if the field exists
      std::any value;
      if (m_node->hasChild(name)) {
        // Try to get the value, but don't assert on type mismatch
        try {
          value = m_node->getValue(name, typeid(void));
        } catch (const std::exception&) {
          // If getValue fails, we'll use the default value
        }
      }

      // If no value was found or getValue failed, initialize with default
      if(!value.has_value()) {
        value = m_node->init(name, description, fmt::format("{}",defaultValue));
      }

      DurationT ret {};

      if(value.type() == typeid(std::chrono::duration<Rep,Period>)) {
        ret = std::any_cast<DurationT>(value);
      } else if(value.type() == typeid(int)) {
        ret = std::chrono::duration<Rep,Period>(std::any_cast<int>(value));
      } else if(value.type() == typeid(double)) {
        if constexpr (std::is_floating_point_v<Rep>) {
          // Direct assignment for floating point representation types
          ret = std::chrono::duration<Rep,Period>(std::any_cast<double>(value));
        } else {
          // For integer representation types, first create a duration with double representation
          // and then cast to the target duration type
          auto doubleValue = std::any_cast<double>(value);
          ret = std::chrono::duration_cast<DurationT>(std::chrono::duration<double>(doubleValue));
        }
      } else if(value.type() == typeid(std::string)) {
        auto optRet = fromStringToDuration<Rep,Period>(std::any_cast<std::string>(value));
        if(!optRet) {
          SPDLOG_ERROR("Duration type {} not recognised",typeName(value.type()));
          throw std::runtime_error("Duration type not recognised");
        }
        ret = *optRet;
      } else {
        SPDLOG_ERROR("Duration type {} not recognised",typeName(value.type()));
        throw std::runtime_error("Duration type not recognised");
      }

      // Check against min/max bounds - throw exception if out of range
      if(ret < minV || ret > maxV) {
        SPDLOG_ERROR("Duration {} out of range {} - {}",ret,minV,maxV);
        throw std::runtime_error("Duration out of range");
      }

      if(update) {
        m_node->setUpdate(name, [update](std::any &newValue) {

          return update(std::any_cast<std::chrono::duration<Rep,Period>>(newValue));
        });
      }
      return ret;
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

    //! Get existing value from node.
    template <typename T>
    [[nodiscard]] T get()
    {
      assert(m_node);
      return m_node->get<T>();
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

    //! Get the node.
    [[nodiscard]] const ConfigNode &node() const
    {
      assert(m_node);
      return *m_node;
    }

    //! Get the node.
    [[nodiscard]] ConfigNode &node()
    {
      assert(m_node);
      return *m_node;
    }
  protected:
    std::shared_ptr<ConfigNode> m_node;
  };

}// namespace Ravl2

