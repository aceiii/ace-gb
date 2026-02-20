#include <expected>
#include <fstream>
#include <iterator>
#include <spdlog/spdlog.h>

#include "file.hpp"


File::LoadFileResult File::LoadBin(const std::string& path) {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  if (input.fail()) {
    return std::unexpected{strerror(errno)};
  }

  input.unsetf(std::ios::skipws);

  input.seekg(0, std::ios::end);
  auto size = input.tellg();
  input.seekg(0, std::ios::beg);

  std::vector<uint8_t> bytes;
  bytes.reserve(size);
  std::copy(std::istream_iterator<uint8_t>(input), std::istream_iterator<uint8_t>(), std::back_inserter(bytes));

  return bytes;
}
