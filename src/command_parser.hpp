#pragma once

#include <string>
#include <string_view>

#include "command.hpp"


namespace CommandParser {
  app::Command Parse(std::string_view line);
}
