#include <set>
#include <utility>

#pragma once

namespace fireplace {

class FireplaceTraits {
 public:
  FireplaceTraits() = default;
  FireplaceTraits(bool power, int power_count)
      : power_(power), power_count_(power_count) {}
  /// Return if this fan supports speed modes.
  bool supports_power() const { return this->power_; }
  /// Set whether this fan supports speed levels.
  void set_power(bool speed) { this->power_ = power; }
  /// Return how many speed levels the fan has
  int supported_power_count() const { return this->power_count_; }
  /// Set how many speed levels this fan has.
  void set_supported_power_count(int power_count) { this->power_count_ = power_count; }
  std::set<std::string> supported_preset_modes() const { return this->preset_modes_; }
  /// Set the preset modes supported by the fan.
  void set_supported_preset_modes(const std::set<std::string> &preset_modes) { this->preset_modes_ = preset_modes; }
  /// Return if preset modes are supported
  bool supports_preset_modes() const { return !this->preset_modes_.empty(); }

 protected:
  bool speed_{false};
  int speed_count_{};
  std::set<std::string> preset_modes_{};
};

}  // namespace fan