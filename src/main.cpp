#include <iostream>
#include <memory>
#include <tracy/Tracy.hpp>

#include "app.hpp"
#include "args.hpp"
#include "headless.hpp"
#include "interface.hpp"

namespace {
  const char* kAppName = "ace-gb";
  const char* kAppVersion = "0.0.1";

  std::unique_ptr<app::IApp> MakeApplication(bool headless) {
    if (headless) {
      return std::make_unique<app::Headless>();
    }
    return std::make_unique<app::Interface>();
  }
}

auto main(int argc, char* argv[]) -> int {
  auto args = app::GetArgs(kAppName, kAppVersion, argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error() << "\n";
    return 1;
  }

  const auto& arg_values = args.value();

  std::unique_ptr<app::IApp> app = MakeApplication(arg_values.headless);

  app->Init(arg_values);
  int result = app->Run();
  app->Cleanup();

  return result;
}
