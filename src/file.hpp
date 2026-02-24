#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "types.hpp"


namespace file {

using U8Buffer = std::vector<u8>;
using LoadFileResult = std::expected<U8Buffer, std::string>;

LoadFileResult LoadBin(std::string_view path);

}
