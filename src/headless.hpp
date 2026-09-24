#pragma once

#include <string>
#include <string_view>

#include "args.hpp"
#include "command.hpp"
#include "emulator.hpp"


namespace app {

  class Headless {
  public:
    void Init(Args args);
    void Run();
    void Cleanup();

  private:
    void Eval(const Command& command);

    Emulator emulator_ {};
  };

}
