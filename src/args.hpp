#pragma once

#include <expected>
#include <string_view>


namespace app {

  struct Args {
    std::string settings_filename;
    std::string log_level;
    bool doctor_log;
  };

  std::expected<Args, std::string> GetArgs(std::string_view name, std::string_view version, int argc, char** argv);

}
