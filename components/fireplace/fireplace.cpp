#include "fireplace.h"
#include "esphome/core/log.h"

namespace fireplace {

static const char *const TAG = "fireplace";

void FireplaceCall::perform() {
  ESP_LOGD(TAG, "'%s' - Setting:", this->parent_.get_name().c_str());
  this->validate_();
  if (this->binary_state_.has_value()) {
    ESP_LOGD(TAG, "  State: %s", ONOFF(*this->binary_state_));
  }
  if (this->power_.has_value()) {
    ESP_LOGD(TAG, "  Power: %d", *this->power_);
  }
  if (!this->preset_mode_.empty()) {
    ESP_LOGD(TAG, "  Preset Mode: %s", this->preset_mode_.c_str());
  }
  this->parent_.control(*this);
}

void FireplaceCall::validate_() {
  auto traits = this->parent_.get_traits();

  if (this->power_.has_value())
    this->power_ = clamp(*this->power_, 1, traits.supported_power_count());

  if (this->binary_state_.has_value() && *this->binary_state_) {
    //TODO should it be last state
    // when turning on, if current speed is zero, set speed to 100%
    if (traits.supports_power() && !this->parent_.state && this->parent_.speed == 0) {
      this->speed_ = traits.supported_speed_count();
    }
  }

  if (this->power_.has_value() && !traits.supports_power()) {
    ESP_LOGW(TAG, "'%s' - This fireplace does not support power levels!", this->parent_.get_name().c_str());
    this->speed_.reset();
  }

  if (!this->preset_mode_.empty()) {
    const auto &preset_modes = traits.supported_preset_modes();
    if (preset_modes.find(this->preset_mode_) == preset_modes.end()) {
      ESP_LOGW(TAG, "'%s' - This fan does not support preset mode '%s'!", this->parent_.get_name().c_str(),
               this->preset_mode_.c_str());
      this->preset_mode_.clear();
    }
  }
}

FireplaceCall FireplaceRestoreState::to_call(Fireplace &fireplace) {
  auto call = fireplace.make_call();
  call.set_state(this->state);
  call.set_power(this->power);

  if (fireplace.get_traits().supports_preset_modes()) {
    // Use stored preset index to get preset name
    const auto &preset_modes = fireplace.get_traits().supported_preset_modes();
    if (this->preset_mode < preset_modes.size()) {
      call.set_preset_mode(*std::next(preset_modes.begin(), this->preset_mode));
    }
  }
  return call;
}
void FireplaceRestoreState::apply(Fireplace &fireplace) {
  fireplace.state = this->state;
  fireplace.power = this->power;

  if (fireplace.get_traits().supports_preset_modes()) {
    // Use stored preset index to get preset name
    const auto &preset_modes = fireplace.get_traits().supported_preset_modes();
    if (this->preset_mode < preset_modes.size()) {
      fireplace.preset_mode = *std::next(preset_modes.begin(), this->preset_mode);
    }
  }
  fireplace.publish_state();
}

FireplaceCall Fireplace::turn_on() { return this->make_call().set_state(true); }
FireplaceCall Fireplace::turn_off() { return this->make_call().set_state(false); }
FireplaceCall Fireplace::toggle() { return this->make_call().set_state(!this->state); }
FireplaceCall Fireplace::make_call() { return FireplaceCall(*this); }

void Fireplace::add_on_state_callback(std::function<void()> &&callback) { this->state_callback_.add(std::move(callback)); }
void Fireplace::publish_state() {
  auto traits = this->get_traits();

  ESP_LOGD(TAG, "'%s' - Sending state:", this->name_.c_str());
  ESP_LOGD(TAG, "  State: %s", ONOFF(this->state));
  if (traits.supports_power()) {
    ESP_LOGD(TAG, "  Power: %d", this->speed);
  }
  if (traits.supports_preset_modes() && !this->preset_mode.empty()) {
    ESP_LOGD(TAG, "  Preset Mode: %s", this->preset_mode.c_str());
  }
  this->state_callback_.call();
  this->save_state_();
}

// Random 32-bit value, change this every time the layout of the FanRestoreState struct changes.
constexpr uint32_t RESTORE_STATE_VERSION = 0x71700ABA;
optional<FireplaceRestoreState> Fireplace::restore_state_() {
  FireplaceRestoreState recovered{};
  this->rtc_ = global_preferences->make_preference<FireplaceRestoreState>(this->get_object_id_hash() ^ RESTORE_STATE_VERSION);
  bool restored = this->rtc_.load(&recovered);

  switch (this->restore_mode_) {
    case FireplaceRestoreMode::NO_RESTORE:
      return {};
    case FireplaceRestoreMode::ALWAYS_OFF:
      recovered.state = false;
      return recovered;
    case FireplaceRestoreMode::ALWAYS_ON:
      recovered.state = true;
      return recovered;
    case FireplaceRestoreMode::RESTORE_DEFAULT_OFF:
      recovered.state = restored ? recovered.state : false;
      return recovered;
    case FireplaceRestoreMode::RESTORE_DEFAULT_ON:
      recovered.state = restored ? recovered.state : true;
      return recovered;
    case FireplaceRestoreMode::RESTORE_INVERTED_DEFAULT_OFF:
      recovered.state = restored ? !recovered.state : false;
      return recovered;
    case FireplaceRestoreMode::RESTORE_INVERTED_DEFAULT_ON:
      recovered.state = restored ? !recovered.state : true;
      return recovered;
  }

  return {};
}
void Fireplace::save_state_() {
  FireplaceRestoreState state{};
  state.state = this->state;
  state.power = this->power;

  if (this->get_traits().supports_preset_modes() && !this->preset_mode.empty()) {
    const auto &preset_modes = this->get_traits().supported_preset_modes();
    // Store index of current preset mode
    auto preset_iterator = preset_modes.find(this->preset_mode);
    if (preset_iterator != preset_modes.end())
      state.preset_mode = std::distance(preset_modes.begin(), preset_iterator);
  }

  this->rtc_.save(&state);
}

void Fireplace::dump_traits_(const char *tag, const char *prefix) {
  auto traits = this->get_traits();

  if (traits.supports_power()) {
    ESP_LOGCONFIG(tag, "%s  Power: YES", prefix);
    ESP_LOGCONFIG(tag, "%s  Power count: %d", prefix, traits.supported_power_count());
  }
  if (traits.supports_preset_modes()) {
    ESP_LOGCONFIG(tag, "%s  Supported presets:", prefix);
    for (const std::string &s : traits.supported_preset_modes())
      ESP_LOGCONFIG(tag, "%s    - %s", prefix, s.c_str());
  }
}

}  // namespace fan