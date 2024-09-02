//
// Created by charles galambos on 27/02/2022.
//

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace Ravl2
{

  //! @brief A thread-safe queue.
  //! Multiple threads can both read and write to the queue safely.  If the maximum size is reached, the writers will
  //! block until the readers have consumed the data.
  //! @tparam DataT the type of the elements in the queue

  template <typename DataT>
  class ThreadedQueue
  {
  public:
    //! @brief Constructor.
    //! @param maxSize the maximum size of the queue.  If the queue is full, the writer will block until the readers have consumed the data.
    explicit ThreadedQueue(size_t maxSize = 10)
        : m_maxSize(maxSize)
    {}

    ThreadedQueue(const ThreadedQueue &) = delete;
    ThreadedQueue &operator=(const ThreadedQueue &) = delete;
    ThreadedQueue(ThreadedQueue &&) = delete;
    ThreadedQueue &operator=(ThreadedQueue &&) = delete;

    //! @brief Destructor.
    ~ThreadedQueue() = default;

    //! @brief Add an element to the queue.
    //! @param data the element to add to the queue
    //! @return true if the element was added to the queue, false if the queue is full
    bool tryPush(const DataT &data)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.size() >= m_maxSize) {
        return false;
      }
      m_queue.push(data);
      m_conditionPop.notify_one();
      return true;
    }

    //! @brief Add an element to the queue.
    //! @param data the element to add to the queue
    //! @return true if the element was added to the queue, false if the queue is full
    bool tryPush(DataT &&data)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.size() >= m_maxSize) {
        return false;
      }
      m_queue.push(std::move(data));
      m_conditionPop.notify_one();
      return true;
    }

    //! @brief Add an element to the queue, blocking if the queue is full.
    //! @param data the element to add to the queue
    void push(const DataT &data)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      while(m_queue.size() >= m_maxSize) {
        m_conditionPush.wait(lock);
      }
      m_queue.push(data);
      m_conditionPop.notify_one();
    }

    //! @brief Add an element to the queue, block until the queue is not full with timeout
    //! @param data the element to add to the queue
    //! @param timeout the timeout in seconds
    //! @return true if the element was added to the queue, false if the timeout expired
    bool tryPush(const DataT &data, double timeout)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.size() >= m_maxSize) {
        if(!m_conditionPush.wait_for(lock, std::chrono::duration<double>(timeout), [this]() { return m_queue.size() < m_maxSize; })) {
          return false;
        }
      }
      m_queue.push(data);
      m_conditionPop.notify_one();
      return true;
    }

    //! @brief Wait for the queue to be non-empty without removing an element.
    //! @param timeout the timeout in seconds
    //! @return true if the queue is non-empty, false if the timeout expired
    [[nodiscard]] bool wait(float timeout = -1.0f) const
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.empty()) {
        if(timeout < 0.0f) {
          m_conditionPop.wait(lock);
        } else {
          if(!m_conditionPop.wait_for(lock, std::chrono::milliseconds(static_cast<int>(timeout * 1000.0f)), [this]() { return !m_queue.empty(); })) {
            return false;
          }
        }
      }
      return true;
    }

    //! @brief Remove an element from the queue.
    //! @param data the element to remove from the queue
    //! @return true if the element was removed from the queue, false if the queue is empty
    [[nodiscard]] bool popWait(DataT &data, float timeout = -1.0f)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.empty()) {
        if(timeout < 0.0f) {
          m_conditionPop.wait(lock);
        } else {
          if(!m_conditionPop.wait_for(lock, std::chrono::milliseconds(static_cast<int>(timeout * 1000.0f)), [this]() { return !m_queue.empty(); })) {
            return false;
          }
        }
      }
      data = m_queue.front();
      m_queue.pop();
      m_conditionPush.notify_one();
      return true;
    }

    //! @brief Try and remove an element from the queue.
    //! @param data the element to remove from the queue
    //! @return true if the element was removed from the queue, false if the queue is empty
    bool tryPop(DataT &data)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_queue.empty()) {
        return false;
      }
      data = std::move(m_queue.front());
      m_queue.pop();
      m_conditionPush.notify_one();
      return true;
    }

    //! @brief Get the number of elements in the queue.
    //! @return the number of elements in the queue
    [[nodiscard]] size_t size() const
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      return m_queue.size();
    }

    //! @brief Get the maximum size of the queue.
    //! @return the maximum size of the queue
    [[nodiscard]] size_t maxSize() const
    {
      return m_maxSize;
    }

    //! @brief Test if the queue is empty
    //! @return true if the queue is empty, false otherwise
    [[nodiscard]] bool empty() const
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      return m_queue.empty();
    }

  protected:
    std::queue<DataT> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_conditionPush;
    mutable std::condition_variable m_conditionPop;
    size_t m_maxSize = 10;
  };

}// namespace Ravl2
