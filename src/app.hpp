#pragma once

#include "args.hpp"

namespace app {

class IApp {
public:
  virtual ~IApp() = default;
  virtual void Init(app::Args args) = 0;
  virtual int Run() = 0;
  virtual void Cleanup() = 0;
};

}
