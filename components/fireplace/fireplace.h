#pragma once

#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/optional.h"
#include "esphome/core/preferences.h"
#include "fan_traits.h"

namespace fireplace {

#define LOG_FIREPLACE(prefix, type, obj) \
  if ((obj) != nullptr) { \
    ESP_LOGCONFIG(TAG, "%s%s '%s'", prefix, LOG_STR_LITERAL(type), (obj)->get_name().c_str()); \
    (obj)->dump_traits_(TAG, prefix); \
  }

/// Restore mode of a fan.
enum class FanRestoreMode {
  NO_RESTORE,
  ALWAYS_OFF,
  ALWAYS_ON,
  RESTORE_DEFAULT_OFF,
  RESTORE_DEFAULT_ON,
  RESTORE_INVERTED_DEFAULT_OFF,
  RESTORE_INVERTED_DEFAULT_ON,
};

class Fireplace;

class FireplaceCall {
 public:
  explicit FireplaceCall(Fireplace &parent) : parent_(parent) {}

  FireplaceCall &set_state(bool binary_state) {
    this->binary_state_ = binary_state;
    return *this;
  }
  FireplaceCall &set_state(optional<bool> binary_state) {
    this->binary_state_ = binary_state;
    return *this;
  }
  optional<bool> get_state() const { return this->binary_state_; }

  optional<int> get_power() const { return this->power_; }

  FanCall &set_preset_mode(const std::string &preset_mode) {
    this->preset_mode_ = preset_mode;
    return *this;
  }
  std::string get_preset_mode() const { return this->preset_mode_; }

  void perform();

 protected:
  void validate_();

  Fireplace &parent_;
  optional<bool> binary_state_;
  optional<int> power_;
  std::string preset_mode_{};
};

struct FanRestoreState {
  bool state;
  int power;
  uint8_t preset_mode;

  /// Convert this struct to a fan call that can be performed.
  FireplaceCall to_call(Fireplace &fireplace);
  /// Apply these settings to the fan.
  void apply(Fireplace &fireplace);
} __attribute__((packed));

class Fireplace : public EntityBase {
 public:
  /// The current on/off state of the fireplace.
  bool state{false};
  /// The current power level of the fireplace, if supported
  int power{0};
  // The current preset mode of the fireplace
  std::string preset_mode{};

  FireplaceCall turn_on();
  FireplaceCall turn_off();
  FireplaceCall toggle();
  FireplaceCall make_call();

  /// Register a callback that will be called each time the state changes.
  void add_on_state_callback(std::function<void()> &&callback);

  void publish_state();

  virtual FireplaceTraits get_traits() = 0;

  /// Set the restore mode of this fan.
  void set_restore_mode(FireplaceRestoreMode restore_mode) { this->restore_mode_ = restore_mode; }

 protected:
  friend FireplaceCall;

  virtual void control(const FireplaceCall &call) = 0;

  optional<FireplaceRestoreState> restore_state_();
  void save_state_();

  void dump_traits_(const char *tag, const char *prefix);

  CallbackManager<void()> state_callback_{};
  ESPPreferenceObject rtc_;
  FireplaceRestoreMode restore_mode_;
};

}  // namespace fireplace