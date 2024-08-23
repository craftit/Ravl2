
#pragma once

#include <string>
#include <span>
#include <vector>
#include <cinttypes>
#include <exception>

namespace Ravl2
{

  class exception_invalid_base64 : public std::exception
  {
  public:
    explicit exception_invalid_base64(const char *message)
        : message_(message)
    {
    }

    [[nodiscard]] const char *what() const noexcept override
    {
      return message_;
    }

  private:
    const char *message_ = nullptr;
  };

  //! @brief Base64 encode some binary data.
  //! @param buffer The binary data to encode.
  //! @return The base64 encoded data.
  [[nodiscard]] std::string base64Encode(const std::span<const std::byte> &buffer);

  //! @brief Base64 encode some binary data.
  //! @param buffer The binary data to encode.
  //! @return The base64 encoded data.
  [[nodiscard]] std::string base64Encode(const std::string_view &buffer);

  //! @brief Base64 decode some data.
  //! @param buffer The base64 encoded data.
  //! @return The decoded data.
  [[nodiscard]] std::vector<std::byte> base64Decode(const std::string_view &buffer);

  //! @brief Base64 decode some data.
  //! @param buffer The base64 encoded data.
  //! @return The decoded data.
  [[nodiscard]] std::string base64DecodeToString(const std::string_view &buffer);

}// namespace Ravl2
