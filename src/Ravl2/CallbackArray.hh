// (C) 2020 React AI Ltd.
// Available under the MIT license

#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <cassert>
#include <mutex>
#include <limits>

namespace Ravl2
{

  template <typename FuncT>
  class CallbackArray;

  //! Abstract callback array base class.
  //! This provides an interface to for managing callback removal

  class AbstractCallbackArray
  {
  public:
    //! Default constructor
    AbstractCallbackArray() = default;

    AbstractCallbackArray(const AbstractCallbackArray &) = delete;
    AbstractCallbackArray &operator=(const AbstractCallbackArray &) = delete;
    AbstractCallbackArray(AbstractCallbackArray &&) = delete;
    AbstractCallbackArray &operator=(AbstractCallbackArray &&) = delete;

    //! virtual destructor to keep the compiler happy.
    virtual ~AbstractCallbackArray() = default;

    //! Remove a callback from the list.
    //! @param id - The id of the callback to remove.
    virtual void remove(size_t id, size_t version) = 0;

  protected:
    size_t m_versionCount = 0;
  };

  //! @brief Handle for a single callback
  //! @details This can be used for managing a callback, removing it when no longer needed.

  class CallbackHandle
  {
  public:
    //! Default constructor,
    //! Creates an invalid handle. It is safe to call Remove() on it which will have no effect.
    CallbackHandle() = default;

    //! Create from an array entry and an id
    CallbackHandle(AbstractCallbackArray *cb, size_t id, size_t version)
        : m_cb(cb),
          m_id(id),
          m_version(version)
    {}

    //! Remove callback from a list if the handle is valid, otherwise take no action.
    //! The handle will be changed to an invalid environment after a call back is removed.
    void remove()
    {
      if(m_cb == nullptr)
        return;
      m_cb->remove(m_id, m_version);
      m_cb = nullptr;
    }

    //! Test if the callback is still active.
    [[nodiscard]] bool isActive() const
    {
      return m_cb != nullptr;
    }

  protected:
    AbstractCallbackArray *m_cb = nullptr;
    size_t m_id = std::numeric_limits<size_t>::max();
    size_t m_version = 0;
  };

  //! @brief Manage a set of callbacks, remove them when destructed.
  //! @details Store a set of call backs that will be removed either when 'RemoveAll()' method is called or when the class is destructed.

  class CallbackSet
  {
  public:
    //! Create an empty callback set.
    CallbackSet() = default;

    //! Disable copy constructor
    CallbackSet(const CallbackSet &) = delete;

    //! Disable copy assignment
    CallbackSet &operator=(const CallbackSet &) = delete;

    //! Destructor,
    // This will disconnect all stored callbacks.
    ~CallbackSet()
    {
      removeAll();
    }

    //! Add a new callback to the set
    CallbackSet &operator+=(const CallbackHandle &handle)
    {
      m_callbacks.push_back(handle);
      return *this;
    }

    //! Remove and disconnect all stored callbacks
    void removeAll();

  private:
    std::vector<CallbackHandle> m_callbacks;
  };

  //! Array of call backs to do on an event.

  template <typename FuncT>
  class CallbackArray : public AbstractCallbackArray
  {
  public:
    //! Default constructor
    CallbackArray() = default;

    //! Make sure we clean up any callbacks
    ~CallbackArray() override
    {
      clear();
    }

    //! Disable copy constructor
    CallbackArray(const CallbackArray &) = delete;
    CallbackArray &operator=(const CallbackArray &) = delete;
    //! Disable move
    CallbackArray(CallbackArray &&) = delete;
    CallbackArray &operator=(CallbackArray &&) = delete;

    //! Get the array of functions
    //! Some handles may be invalid, so they need to be checked before calling.
    [[nodiscard]] std::vector<FuncT> calls() const
    {
      std::lock_guard lock(m_mutexAccess);
      std::vector<FuncT> ret;
      ret.reserve(m_callbacks.size());
      for(auto &cb : m_callbacks) {
        if(cb.m_callback)
          ret.push_back(cb.m_callback);
      }
      return ret;
    }

    //! Add a new call back to the list.

    CallbackHandle add(const FuncT &callback)
    {
      std::lock_guard lock(m_mutexAccess);
      m_versionCount++;
      // Free slot anywhere?
      for(size_t i = 0; i < m_callbacks.size(); i++) {
        if(!m_callbacks[i].m_callback) {
          m_callbacks[i] = {callback, m_versionCount};
          return CallbackHandle(this, i, m_versionCount);
        }
      }
      // Just create a new one
      auto ret = m_callbacks.size();
      m_callbacks.push_back({callback, m_versionCount});
      return CallbackHandle(this, ret, m_versionCount);
    }

    //! Move in a new call back to the list.
    CallbackHandle add(FuncT &&callback)
    {
      std::lock_guard lock(m_mutexAccess);
      m_versionCount++;
      // Free slot anywhere?
      for(size_t i = 0; i < m_callbacks.size(); i++) {
        auto &cb = m_callbacks[i];
        if(!cb.m_callback) {
          cb.m_callback = std::move(callback);
          cb.m_version = m_versionCount;
          return CallbackHandle(this, i, m_versionCount);
        }
      }
      // Just create a new one
      auto ret = m_callbacks.size();
      m_callbacks.push_back({std::move(callback), m_versionCount});
      return CallbackHandle(this, ret, m_versionCount);
    }

    //! Remove a callback by id
    void remove(size_t id, size_t version) override
    {
      std::lock_guard lock(m_mutexAccess);
      assert(id < m_callbacks.size());
      auto &cb = m_callbacks[id];
      if(cb.m_version == version) {
        cb.m_callback = FuncT();
      }
    }

    //! Remove all callbacks
    void clear()
    {
      std::lock_guard lock(m_mutexAccess);
      for(auto &cb : m_callbacks) {
        cb.m_callback = FuncT();
      }
    }

    //! Call a set of methods.
    //! Returns true if at least one method was called.
    template <typename... Args>
    bool call(Args &&...args) const
    {
      auto callList = calls();
      bool ret = false;
      for(auto &x : callList) {
        if(x) {
          x(std::forward<Args>(args)...);
          ret = true;// Has handler.
        }
      }
      return ret;
    }

    //! Test if the callback list is empty
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(m_mutexAccess);
      if(m_callbacks.empty())
        return true;
      for(auto &cb : m_callbacks) {
        if(cb.m_callback)
          return false;
      }
      return true;
    }

  private:
    mutable std::mutex m_mutexAccess;
    struct CallbackData {
      FuncT m_callback;
      size_t m_version;
    };
    std::vector<CallbackData> m_callbacks;
  };

}// namespace Ravl2
