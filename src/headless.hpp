#pragma once

#include <string>
#include <string_view>

#include "app.hpp"
#include "args.hpp"
#include "command.hpp"
#include "emulator.hpp"


namespace app {

  class Headless : public app::IApp {
  public:
    ~Headless() override = default;

    void Init(Args args) override;
    int Run() override;
    void Cleanup() override;

  private:
    void Eval(const Command& command);

    Emulator emulator_ {};
  };

}
