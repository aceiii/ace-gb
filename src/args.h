#pragma once

#include <expected>
#include <string>


namespace app {

  struct Args {
    std::string settings_filename;
    std::string log_level;
    bool doctor_log;
  };

  std::expected<Args, std::string> GetArgs(const std::string& name, const std::string& version, int argc, char** argv);

}
