//
// Created by charles on 25/08/24.
//

#pragma once

#include <vector>
#include "Ravl2/IO/StreamInput.hh"

namespace Ravl2
{
  //! @brief A stream that just reads from a vector in memory.
  template <typename ObjectT, typename ContainerT = std::vector<ObjectT>>
  class InputStreamContainer : public StreamInput<ObjectT>
  {
  public:
    InputStreamContainer() = default;

    //! Construct with a vector of objects.
    explicit InputStreamContainer(std::vector<ObjectT> objects)
        : m_objects(std::move(objects))
    {}

    //! Construct with a single object.
    explicit InputStreamContainer(ObjectT &&object)
        : m_objects {std::move(object)}
    {}

    //! Get object.
    std::optional<ObjectT> next(std::streampos &pos) final
    {
      if(pos < 0) {
        return std::nullopt;
      }
      if(size_t(pos) >= m_objects.size()) {
        return std::nullopt;
      }
      auto wasAt = pos;
      pos += 1;
      return m_objects[static_cast<size_t>(wasAt)];
    }

  private:
    ContainerT m_objects;
  };

  extern template class InputStreamContainer<std::string>;
}// namespace Ravl2