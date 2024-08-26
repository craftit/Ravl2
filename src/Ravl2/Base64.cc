//
// Created by charles on 25/02/23.
//

#include <exception>
#include <spdlog/spdlog.h>

#include "Ravl2/Base64.hh"

namespace Ravl2
{

  static char *buildEncodeTable();
  static const std::byte *buildDecodeTable();

  static const char *encodeTable = buildEncodeTable();
  static const std::byte *decodeTable = buildDecodeTable();

  static char *buildEncodeTable()
  {
    static char dtable[256];
    int i;
    for(i = 0; i < 9; i++) {
      dtable[i] = 'A' + static_cast<char>(i);
      dtable[i + 9] = 'J' + static_cast<char>(i);
      dtable[26 + i] = 'a' + static_cast<char>(i);
      dtable[26 + i + 9] = 'j' + static_cast<char>(i);
    }
    for(i = 0; i < 8; i++) {
      dtable[i + 18] = 'S' + static_cast<char>(i);
      dtable[26 + i + 18] = 's' + static_cast<char>(i);
    }
    for(i = 0; i < 10; i++)
      dtable[52 + i] = '0' + static_cast<char>(i);
    dtable[62] = '+';
    dtable[63] = '/';
    return dtable;
  }

  static const std::byte *buildDecodeTable()
  {
    static std::byte dtable[256];
    uint8_t i;
    for(i = 0; i < 255; i++)
      dtable[i] = std::byte(0x80);
    for(i = 'A'; i <= 'I'; i++)
      dtable[i] = std::byte(0 + (i - 'A'));
    for(i = 'J'; i <= 'R'; i++)
      dtable[i] = std::byte(9 + (i - 'J'));
    for(i = 'S'; i <= 'Z'; i++)
      dtable[i] = std::byte(18 + (i - 'S'));
    for(i = 'a'; i <= 'i'; i++)
      dtable[i] = std::byte(26 + (i - 'a'));
    for(i = 'j'; i <= 'r'; i++)
      dtable[i] = std::byte(35 + (i - 'j'));
    for(i = 's'; i <= 'z'; i++)
      dtable[i] = std::byte(44 + (i - 's'));
    for(i = '0'; i <= '9'; i++)
      dtable[i] = std::byte(52 + (i - '0'));
    dtable[int('+')] = std::byte(62);
    dtable[int('/')] = std::byte(63);
    dtable[int('=')] = std::byte(0);
    return dtable;
  }

  //: Encode buffer.

  std::string
  base64Encode(const std::span<const std::byte> &buffer)
  {
    std::string ret;
    ret.reserve(static_cast<size_t>(static_cast<double>(buffer.size()) * 1.41) + 4);
    std::byte igroup[3];
    char ogroup[5];
    ogroup[4] = 0;
    int n, ln = 0;
    for(auto it = buffer.begin(); it != buffer.end();) {
      igroup[0] = std::byte(0);
      igroup[1] = std::byte(0);
      igroup[2] = std::byte(0);
      for(n = 0; n < 3 && (it != buffer.end()); n++, it++)
        igroup[n] = *it;
      ogroup[0] = encodeTable[unsigned(igroup[0]) >> 2];
      ogroup[1] = encodeTable[((unsigned(igroup[0]) & 3) << 4) | (unsigned(igroup[1]) >> 4)];
      ogroup[2] = encodeTable[((unsigned(igroup[1]) & 0xF) << 2) | (unsigned(igroup[2]) >> 6)];
      ogroup[3] = encodeTable[unsigned(igroup[2]) & 0x3F];
      if(n < 3) {
        ogroup[3] = '=';
        if(n < 2) {
          ogroup[2] = '=';
        }
      }
      ret.append(ogroup);
      if(ln++ >= 18) {
        ln = 0;
        ret += '\n';
      }
    }
    return ret;
  }

  std::string base64Encode(const std::string_view &buffer)
  {
    return base64Encode(std::span<const std::byte>(reinterpret_cast<const std::byte *>(buffer.data()), buffer.size()));
  }

  //: Decode a string into raw binary.

  std::vector<std::byte> base64Decode(const std::string_view &buffer)
  {
    if(buffer.empty())
      return {};
    std::vector<std::byte> ret;
    ret.reserve(static_cast<size_t>(static_cast<double>(buffer.size()) * 0.75) + 4);
    auto at = buffer.begin();
    auto end = buffer.end();
    std::byte a[4], b[4];
    int i;
    while(at != end) {
      for(i = 0; i < 4; i++, at++) {
        if(at == end) {
          if(i == 0) {// Just some white space at the end of the string ?
                      // Easier just to return here, rather than escape from the loop.
            return ret;
          }
          SPDLOG_ERROR("Base64C::Decode(), Unexpected end of string. ");
          throw exception_invalid_base64("Base64C::Decode(), Unexpected end of string. ");
        }
        unsigned ind = static_cast<unsigned>(uint8_t(*at));
        assert(ind < 256);
        std::byte dc = decodeTable[ind];
        if(int(dc) & 0x80) {
          i--;
          continue;
        }
        a[i] = static_cast<std::byte>(*at);
        b[i] = dc;
      }
      ret.push_back(((b[0] << 2) | (b[1] >> 4)));
      if(a[2] == std::byte('=')) break;
      ret.push_back(((b[1] << 4) | (b[2] >> 2)));
      if(a[3] == std::byte('=')) break;
      ret.push_back(((b[2] << 6) | b[3]));
    }
    return ret;
  }

  std::string base64DecodeToString(const std::string_view &buffer)
  {
    auto ret = base64Decode(buffer);
    return {reinterpret_cast<const char *>(ret.data()), ret.size()};
  }

}// namespace Ravl2