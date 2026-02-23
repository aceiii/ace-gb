#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>


namespace file {

using U8Buffer = std::vector<uint8_t>;
using LoadFileResult = std::expected<U8Buffer, std::string>;

LoadFileResult LoadBin(std::string_view path);

}
