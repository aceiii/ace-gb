#include <iostream>
#include <tracy/Tracy.hpp>

#include "args.hpp"
#include "headless.hpp"
#include "interface.hpp"

namespace {
  const char* kAppName = "ace-gb";
  const char* kAppVersion = "0.0.1";
}

auto main(int argc, char* argv[]) -> int {
  auto args = app::GetArgs(kAppName, kAppVersion, argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error() << "\n";
    return 1;
  }

  const auto& arg_values = args.value();

  if (arg_values.headless) {
    app::Headless headless;
    headless.Init(arg_values);
    headless.Run();
    headless.Cleanup();
  } else {
    app::Interface interface;
    interface.Init(arg_values);
    interface.Run();
    interface.Cleanup();
  }

  return 0;
}
