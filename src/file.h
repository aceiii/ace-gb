#pragma once

#include <expected>
#include <string>
#include <vector>

namespace File {

using U8Buffer = std::vector<uint8_t>;
using LoadFileResult = std::expected<U8Buffer, std::string>;

LoadFileResult LoadBin(const std::string &path);

}
