#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "fireplace_state.h"

namespace fireplace {

template<typename... Ts> class TurnOnAction : public Action<Ts...> {
 public:
  explicit TurnOnAction(Fireplace *state) : state_(state) {}

  TEMPLATABLE_VALUE(bool, oscillating)
  TEMPLATABLE_VALUE(int, speed)
  TEMPLATABLE_VALUE(FireplaceDirection, direction)

  void play(Ts... x) override {
    auto call = this->state_->turn_on();
    if (this->oscillating_.has_value()) {
      call.set_oscillating(this->oscillating_.value(x...));
    }
    if (this->speed_.has_value()) {
      call.set_speed(this->speed_.value(x...));
    }
    if (this->direction_.has_value()) {
      call.set_direction(this->direction_.value(x...));
    }
    call.perform();
  }

  Fireplace *state_;
};

template<typename... Ts> class TurnOffAction : public Action<Ts...> {
 public:
  explicit TurnOffAction(Fireplace *state) : state_(state) {}

  void play(Ts... x) override { this->state_->turn_off().perform(); }

  Fireplace *state_;
};

template<typename... Ts> class ToggleAction : public Action<Ts...> {
 public:
  explicit ToggleAction(Fireplace *state) : state_(state) {}

  void play(Ts... x) override { this->state_->toggle().perform(); }

  Fireplace *state_;
};

template<typename... Ts> class CycleSpeedAction : public Action<Ts...> {
 public:
  explicit CycleSpeedAction(Fireplace *state) : state_(state) {}

  TEMPLATABLE_VALUE(bool, no_off_cycle)

  void play(Ts... x) override {
    // check to see if fan supports speeds and is on
    if (this->state_->get_traits().supported_speed_count()) {
      if (this->state_->state) {
        int speed = this->state_->speed + 1;
        int supported_speed_count = this->state_->get_traits().supported_speed_count();
        bool off_speed_cycle = no_off_cycle_.value(x...);
        if (speed > supported_speed_count && off_speed_cycle) {
          // was running at max speed, off speed cycle enabled, so turn off
          speed = 1;
          auto call = this->state_->turn_off();
          call.set_speed(speed);
          call.perform();
        } else if (speed > supported_speed_count && !off_speed_cycle) {
          // was running at max speed, off speed cycle disabled, so set to lowest speed
          auto call = this->state_->turn_on();
          call.set_speed(1);
          call.perform();
        } else {
          auto call = this->state_->turn_on();
          call.set_speed(speed);
          call.perform();
        }
      } else {
        // fan was off, so set speed to 1
        auto call = this->state_->turn_on();
        call.set_speed(1);
        call.perform();
      }
    } else {
      // fan doesn't support speed counts, so toggle
      this->state_->toggle().perform();
    }
  }

  Fireplace *state_;
};

template<typename... Ts> class FireplaceIsOnCondition : public Condition<Ts...> {
 public:
  explicit FireplaceIsOnCondition(Fireplace *state) : state_(state) {}
  bool check(Ts... x) override { return this->state_->state; }

 protected:
  Fireplace *state_;
};
template<typename... Ts> class FireplaceIsOffCondition : public Condition<Ts...> {
 public:
  explicit FireplaceIsOffCondition(Fireplace *state) : state_(state) {}
  bool check(Ts... x) override { return !this->state_->state; }

 protected:
  Fireplace *state_;
};

class FireplaceTurnOnTrigger : public Trigger<> {
 public:
  FireplaceTurnOnTrigger(Fireplace *state) {
    state->add_on_state_callback([this, state]() {
      auto is_on = state->state;
      auto should_trigger = is_on && !this->last_on_;
      this->last_on_ = is_on;
      if (should_trigger) {
        this->trigger();
      }
    });
    this->last_on_ = state->state;
  }

 protected:
  bool last_on_;
};

class FireplaceTurnOffTrigger : public Trigger<> {
 public:
  FireplaceTurnOffTrigger(Fireplace *state) {
    state->add_on_state_callback([this, state]() {
      auto is_on = state->state;
      auto should_trigger = !is_on && this->last_on_;
      this->last_on_ = is_on;
      if (should_trigger) {
        this->trigger();
      }
    });
    this->last_on_ = state->state;
  }

 protected:
  bool last_on_;
};

class FireplaceSpeedSetTrigger : public Trigger<> {
 public:
  FireplaceSpeedSetTrigger(Fireplace *state) {
    state->add_on_state_callback([this, state]() {
      auto speed = state->speed;
      auto should_trigger = speed != !this->last_speed_;
      this->last_speed_ = speed;
      if (should_trigger) {
        this->trigger();
      }
    });
    this->last_speed_ = state->speed;
  }

 protected:
  int last_speed_;
};

class FireplacePresetSetTrigger : public Trigger<> {
 public:
  FireplacePresetSetTrigger(Fireplace *state) {
    state->add_on_state_callback([this, state]() {
      auto preset_mode = state->preset_mode;
      auto should_trigger = preset_mode != this->last_preset_mode_;
      this->last_preset_mode_ = preset_mode;
      if (should_trigger) {
        this->trigger();
      }
    });
    this->last_preset_mode_ = state->preset_mode;
  }

 protected:
  std::string last_preset_mode_;
};

}  // namespace fan