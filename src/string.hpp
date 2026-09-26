#pragma once

#include <string>
#include <string_view>


namespace string {

  static constexpr std::string kWhiteSpaceChars {" \t\n\r\x0b\x0c"};

  std::string_view Trim(std::string_view input, std::string_view chars = kWhiteSpaceChars);
  std::string_view TrimLeading(std::string_view input, std::string_view chars = kWhiteSpaceChars);
  std::string_view TrimTrailing(std::string_view input, std::string_view chars = kWhiteSpaceChars);

}
