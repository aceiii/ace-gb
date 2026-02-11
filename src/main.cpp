#include <iostream>

#include "args.h"
#include "interface.h"

namespace {
  const char* kAppName = "ace-gb";
  const char* kAppVersion = "0.0.1";
}

auto main(int argc, char *argv[]) -> int {
  auto args = app::GetArgs(kAppName, kAppVersion, argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error() << "\n";
    return 1;
  }

  app::Interface interface(args.value());
  interface.run();
  return 0;
}
