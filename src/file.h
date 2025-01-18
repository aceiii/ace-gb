#pragma once

#include <string>
#include <vector>
#include <tl/expected.hpp>

using byte_buffer = std::vector<uint8_t>;
using load_file_result = tl::expected<byte_buffer, std::string>;

load_file_result load_bin(const std::string &path);
