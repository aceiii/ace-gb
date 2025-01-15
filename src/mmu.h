#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

struct addr_range {
  uint16_t start;
  uint16_t end;
};

class IMMUDevice {
public:
  virtual void write8(uint16_t addr, uint8_t byte) = 0;
  virtual void write16(uint16_t addr, uint16_t word) = 0;

  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint16_t read16(uint16_t addr) const = 0;

  virtual void reset() = 0;
};

class MMU {
public:
  void add_device(addr_range range, std::shared_ptr<IMMUDevice> &&device);

  void write8(uint16_t addr, uint8_t byte);
  void write16(uint16_t addr, uint16_t word);

  [[nodiscard]] uint8_t read8(uint16_t addr) const;
  [[nodiscard]] uint16_t read16(uint16_t addr) const;

  void reset_devices();

private:
  std::vector<std::pair<addr_range, std::shared_ptr<IMMUDevice>>> devices_;
};

