import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import mqtt
from esphome.const import (
    CONF_ID,
    CONF_MQTT_ID,
    CONF_OSCILLATING,
    CONF_OSCILLATION_COMMAND_TOPIC,
    CONF_OSCILLATION_STATE_TOPIC,
    CONF_SPEED,
    CONF_SPEED_LEVEL_COMMAND_TOPIC,
    CONF_SPEED_LEVEL_STATE_TOPIC,
    CONF_SPEED_COMMAND_TOPIC,
    CONF_SPEED_STATE_TOPIC,
    CONF_OFF_SPEED_CYCLE,
    CONF_ON_SPEED_SET,
    CONF_ON_TURN_OFF,
    CONF_ON_TURN_ON,
    CONF_ON_PRESET_SET,
    CONF_TRIGGER_ID,
    CONF_DIRECTION,
    CONF_RESTORE_MODE,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.cpp_helpers import setup_entity

IS_PLATFORM_COMPONENT = True

fireplace_ns = cg.esphome_ns.namespace("fireplace")
Fireplace = fireplace_ns.class_("Fireplace", cg.EntityBase)
FireplaceState = fireplace_ns.class_("Fireplace", Fireplace, cg.Component)

FireplaceRestoreMode = fireplace_ns.enum("FireplaceRestoreMode", is_class=True)
RESTORE_MODES = {
    "NO_RESTORE": FireplaceRestoreMode.NO_RESTORE,
    "ALWAYS_OFF": FireplaceRestoreMode.ALWAYS_OFF,
    "ALWAYS_ON": FireplaceRestoreMode.ALWAYS_ON,
    "RESTORE_DEFAULT_OFF": FireplaceRestoreMode.RESTORE_DEFAULT_OFF,
    "RESTORE_DEFAULT_ON": FireplaceRestoreMode.RESTORE_DEFAULT_ON,
    "RESTORE_INVERTED_DEFAULT_OFF": FireplaceRestoreMode.RESTORE_INVERTED_DEFAULT_OFF,
    "RESTORE_INVERTED_DEFAULT_ON": FireplaceRestoreMode.RESTORE_INVERTED_DEFAULT_ON,
}

# Actions
TurnOnAction = fireplace_ns.class_("TurnOnAction", automation.Action)
TurnOffAction = fireplace_ns.class_("TurnOffAction", automation.Action)
ToggleAction = fireplace_ns.class_("ToggleAction", automation.Action)
CycleSpeedAction = fireplace_ns.class_("CycleSpeedAction", automation.Action)

FireplaceTurnOnTrigger = fireplace_ns.class_("FireplaceTurnOnTrigger", automation.Trigger.template())
FireplaceTurnOffTrigger = fireplace_ns.class_("FireplaceTurnOffTrigger", automation.Trigger.template())
FireplaceSpeedSetTrigger = fireplace_ns.class_("FireplaceSpeedSetTrigger", automation.Trigger.template())
FireplacePresetSetTrigger = fireplace_ns.class_(
    "FireplacePresetSetTrigger", automation.Trigger.template()
)

FireplaceIsOnCondition = fireplace_ns.class_("FireplaceIsOnCondition", automation.Condition.template())
FireplaceIsOffCondition = fireplace_ns.class_("FireplaceIsOffCondition", automation.Condition.template())

FAN_SCHEMA = cv.ENTITY_BASE_SCHEMA.extend(cv.MQTT_COMMAND_COMPONENT_SCHEMA).extend(
    {
        cv.GenerateID(): cv.declare_id(Fireplace),
        cv.Optional(CONF_RESTORE_MODE, default="ALWAYS_OFF"): cv.enum(
            RESTORE_MODES, upper=True, space="_"
        ),
        cv.OnlyWith(CONF_MQTT_ID, "mqtt"): cv.declare_id(mqtt.MQTTFireplaceComponent),
        cv.Optional(CONF_POWER_LEVEL_STATE_TOPIC): cv.All(
            cv.requires_component("mqtt"), cv.publish_topic
        ),
        cv.Optional(CONF_POWER_LEVEL_COMMAND_TOPIC): cv.All(
            cv.requires_component("mqtt"), cv.subscribe_topic
        ),
        cv.Optional(CONF_POWER_STATE_TOPIC): cv.All(
            cv.requires_component("mqtt"), cv.publish_topic
        ),
        cv.Optional(CONF_POWER_COMMAND_TOPIC): cv.All(
            cv.requires_component("mqtt"), cv.subscribe_topic
        ),
        cv.Optional(CONF_ON_TURN_ON): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FireplaceTurnOnTrigger),
            }
        ),
        cv.Optional(CONF_ON_TURN_OFF): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FireplaceTurnOffTrigger),
            }
        ),
        cv.Optional(CONF_ON_POWER_SET): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FireplaceSpeedSetTrigger),
            }
        ),
        cv.Optional(CONF_ON_PRESET_SET): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FireplacePresetSetTrigger),
            }
        ),
    }
)

_PRESET_MODES_SCHEMA = cv.All(
    cv.ensure_list(cv.string_strict),
    cv.Length(min=1),
)


def validate_preset_modes(value):
    # Check against defined schema
    value = _PRESET_MODES_SCHEMA(value)

    # Ensure preset names are unique
    errors = []
    presets = set()
    for i, preset in enumerate(value):
        # If name does not exist yet add it
        if preset not in presets:
            presets.add(preset)
            continue

        # Otherwise it's an error
        errors.append(
            cv.Invalid(
                f"Found duplicate preset name '{preset}'. Presets must have unique names.",
                [i],
            )
        )

    if errors:
        raise cv.MultipleInvalid(errors)

    return value


async def setup_fireplace_core_(var, config):
    await setup_entity(var, config)

    cg.add(var.set_restore_mode(config[CONF_RESTORE_MODE]))

    if CONF_MQTT_ID in config:
        mqtt_ = cg.new_Pvariable(config[CONF_MQTT_ID], var)
        await mqtt.register_mqtt_component(mqtt_, config)

        if CONF_POWER_LEVEL_STATE_TOPIC in config:
            cg.add(
                mqtt_.set_custom_speed_level_state_topic(
                    config[CONF_POWER_LEVEL_STATE_TOPIC]
                )
            )
        if CONF_POWER_LEVEL_COMMAND_TOPIC in config:
            cg.add(
                mqtt_.set_custom_speed_level_command_topic(
                    config[CONF_POWER_LEVEL_COMMAND_TOPIC]
                )
            )
        if CONF_POWER_STATE_TOPIC in config:
            cg.add(mqtt_.set_custom_speed_state_topic(config[CONF_POWER_STATE_TOPIC]))
        if CONF_POWER_COMMAND_TOPIC in config:
            cg.add(
                mqtt_.set_custom_speed_command_topic(config[CONF_POWER_COMMAND_TOPIC])
            )

    for conf in config.get(CONF_ON_TURN_ON, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_TURN_OFF, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_POWER_SET, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_PRESET_SET, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


async def register_fireplace(var, config):
    if not CORE.has_id(config[CONF_ID]):
        var = cg.Pvariable(config[CONF_ID], var)
    cg.add(cg.App.register_fireplace(var))
    await setup_fireplace_core_(var, config)


async def create_fireplace_state(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await register_fireplace(var, config)
    await cg.register_component(var, config)
    return var


FIREPLACE_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(Fireplace),
    }
)


@automation.register_action("fireplace.toggle", ToggleAction, FIREPLACE_ACTION_SCHEMA)
async def fireplace_toggle_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action("fireplace.turn_off", TurnOffAction, FIREPLACE_ACTION_SCHEMA)
async def fireplace_turn_off_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "fireplace.turn_on",
    TurnOnAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(Fireplace),
            cv.Optional(CONF_POWER): cv.templatable(cv.int_range(1)),
        }
    ),
)
async def fireplace_turn_on_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    if CONF_POWER in config:
        template_ = await cg.templatable(config[CONF_POWER], args, int)
        cg.add(var.set_speed(template_))
    return var



@automation.register_condition(
    "fireplace.is_on",
    FireplaceIsOnCondition,
    automation.maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(Fireplace),
        }
    ),
)
@automation.register_condition(
    "fireplace.is_off",
    FireplaceIsOffCondition,
    automation.maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(Fireplace),
        }
    ),
)
async def fireplace_is_on_off_to_code(config, condition_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, paren)


@coroutine_with_priority(100.0)
async def to_code(config):
    cg.add_define("USE_FIREPLACE")
    cg.add_global(fireplace_ns.using)