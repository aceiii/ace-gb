#include <expected>
#include <fstream>
#include <iterator>
#include <spdlog/spdlog.h>

#include "file.hpp"


file::LoadFileResult file::LoadBin(std::string_view path) {
  std::ifstream input(std::string{path}, std::ios::in | std::ios::binary);
  if (input.fail()) {
    return std::unexpected{strerror(errno)};
  }

  input.unsetf(std::ios::skipws);

  input.seekg(0, std::ios::end);
  auto size = input.tellg();
  input.seekg(0, std::ios::beg);

  std::vector<u8> bytes;
  bytes.reserve(size);
  std::copy(std::istream_iterator<u8>(input), std::istream_iterator<u8>(), std::back_inserter(bytes));

  return bytes;
}
