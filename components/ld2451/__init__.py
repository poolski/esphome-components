import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, number, select, sensor, text_sensor, uart
from esphome.const import CONF_ID, DEVICE_CLASS_DISTANCE, DEVICE_CLASS_MOTION, ENTITY_CATEGORY_CONFIG

CODEOWNERS = ["@poolski"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "number", "select"]
MULTI_CONF = True

CONF_TARGET_COUNT = "target_count"
CONF_ALARM = "alarm"
CONF_ANGLE = "angle"
CONF_DISTANCE = "distance"
CONF_SPEED = "speed"
CONF_SNR = "snr"
CONF_DIRECTION = "direction"
CONF_MAX_DISTANCE = "max_distance"
CONF_MIN_DISTANCE = "min_distance"
CONF_MIN_SPEED = "min_speed"
CONF_DETECTION_DIRECTION = "detection_direction"
CONF_NO_TARGET_DELAY = "no_target_delay"
CONF_TRIGGER_COUNT = "trigger_count"
CONF_MIN_SNR = "min_snr"
CONF_APP_SNR_THRESHOLD = "app_snr_threshold"
CONF_SPEED_CORRECTION = "speed_correction"
CONF_CONTROLS = "controls"

DETECTION_DIRECTION_OPTIONS = {
    "away": 0,
    "approach": 1,
    "both": 2,
}

ld2451_ns = cg.esphome_ns.namespace("ld2451")
LD2451Component = ld2451_ns.class_("LD2451Component", cg.Component, uart.UARTDevice)
LD2451MaxDistanceNumber = ld2451_ns.class_(
    "LD2451MaxDistanceNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451MinDistanceNumber = ld2451_ns.class_(
    "LD2451MinDistanceNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451MinSpeedNumber = ld2451_ns.class_(
    "LD2451MinSpeedNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451NoTargetDelayNumber = ld2451_ns.class_(
    "LD2451NoTargetDelayNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451TriggerCountNumber = ld2451_ns.class_(
    "LD2451TriggerCountNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451MinSnrNumber = ld2451_ns.class_(
    "LD2451MinSnrNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451AppSnrThresholdNumber = ld2451_ns.class_(
    "LD2451AppSnrThresholdNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451SpeedCorrectionNumber = ld2451_ns.class_(
    "LD2451SpeedCorrectionNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451DetectionDirectionSelect = ld2451_ns.class_(
    "LD2451DetectionDirectionSelect", select.Select, cg.Parented.template(LD2451Component)
)
cg.add_global(ld2451_ns.using)

def _validate_distance_window(config):
    min_distance = config.get(CONF_MIN_DISTANCE, 0)
    max_distance = config.get(CONF_MAX_DISTANCE, 100)
    if min_distance > max_distance:
        raise cv.Invalid("min_distance must be <= max_distance")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LD2451Component),
            cv.Optional(CONF_MAX_DISTANCE): cv.int_range(min=10, max=100),
            cv.Optional(CONF_MIN_DISTANCE, default=0): cv.int_range(min=0, max=100),
            cv.Optional(CONF_MIN_SPEED): cv.int_range(min=0, max=120),
            cv.Optional(CONF_DETECTION_DIRECTION): cv.enum(DETECTION_DIRECTION_OPTIONS, lower=True),
            cv.Optional(CONF_NO_TARGET_DELAY): cv.int_range(min=0, max=255),
            cv.Optional(CONF_TRIGGER_COUNT): cv.int_range(min=1, max=10),
            cv.Optional(CONF_MIN_SNR): cv.Any(cv.int_range(min=0, max=0), cv.int_range(min=3, max=8)),
            cv.Optional(CONF_SPEED_CORRECTION, default=1.0): cv.float_range(min=0.1, max=4.0),
            cv.Optional(CONF_CONTROLS): cv.Schema(
                {
                    cv.Optional(CONF_MAX_DISTANCE): number.number_schema(
                        LD2451MaxDistanceNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:ruler",
                    ),
                    cv.Optional(CONF_MIN_DISTANCE): number.number_schema(
                        LD2451MinDistanceNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:ruler-square",
                    ),
                    cv.Optional(CONF_MIN_SPEED): number.number_schema(
                        LD2451MinSpeedNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:speedometer",
                    ),
                    cv.Optional(CONF_NO_TARGET_DELAY): number.number_schema(
                        LD2451NoTargetDelayNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:timer-outline",
                    ),
                    cv.Optional(CONF_TRIGGER_COUNT): number.number_schema(
                        LD2451TriggerCountNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:counter",
                    ),
                    cv.Optional(CONF_MIN_SNR): number.number_schema(
                        LD2451MinSnrNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:signal",
                    ),
                    cv.Optional(CONF_APP_SNR_THRESHOLD): number.number_schema(
                        LD2451AppSnrThresholdNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:signal-distance-variant",
                    ),
                    cv.Optional(CONF_SPEED_CORRECTION): number.number_schema(
                        LD2451SpeedCorrectionNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:tune-variant",
                    ),
                    cv.Optional(CONF_DETECTION_DIRECTION): select.select_schema(
                        LD2451DetectionDirectionSelect,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:sign-direction",
                    ),
                }
            ),
            cv.Optional(CONF_TARGET_COUNT): sensor.sensor_schema(
                unit_of_measurement="targets",
                accuracy_decimals=0,
                icon="mdi:counter",
            ),
            cv.Optional(CONF_ALARM): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOTION,
                icon="mdi:motion-sensor",
            ),
            cv.Optional(CONF_ANGLE): sensor.sensor_schema(
                unit_of_measurement="deg",
                accuracy_decimals=0,
                icon="mdi:angle-obtuse",
            ),
            cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
                unit_of_measurement="m",
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DISTANCE,
                icon="mdi:map-marker-distance",
            ),
            cv.Optional(CONF_SPEED): sensor.sensor_schema(
                unit_of_measurement="km/h",
                accuracy_decimals=2,
                icon="mdi:speedometer",
            ),
            cv.Optional(CONF_SNR): sensor.sensor_schema(
                unit_of_measurement="dB",
                accuracy_decimals=0,
                icon="mdi:signal",
            ),
            cv.Optional(CONF_DIRECTION): text_sensor.text_sensor_schema(
                icon="mdi:sign-direction",
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    _validate_distance_window,
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "ld2451",
    baud_rate=115200,
    require_tx=False,
    require_rx=True,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_MAX_DISTANCE in config:
        cg.add(var.set_max_distance(config[CONF_MAX_DISTANCE]))
    if CONF_MIN_DISTANCE in config:
        cg.add(var.set_min_distance(config[CONF_MIN_DISTANCE]))
    if CONF_MIN_SPEED in config:
        cg.add(var.set_min_speed(config[CONF_MIN_SPEED]))
    if CONF_DETECTION_DIRECTION in config:
        cg.add(var.set_detection_direction(config[CONF_DETECTION_DIRECTION]))
    if CONF_NO_TARGET_DELAY in config:
        cg.add(var.set_no_target_delay(config[CONF_NO_TARGET_DELAY]))
    if CONF_TRIGGER_COUNT in config:
        cg.add(var.set_trigger_count(config[CONF_TRIGGER_COUNT]))
    if CONF_MIN_SNR in config:
        cg.add(var.set_min_snr(config[CONF_MIN_SNR]))
    if CONF_SPEED_CORRECTION in config:
        cg.add(var.set_speed_correction(config[CONF_SPEED_CORRECTION]))

    if controls_config := config.get(CONF_CONTROLS):
        if max_distance_config := controls_config.get(CONF_MAX_DISTANCE):
            n = await number.new_number(max_distance_config, min_value=10, max_value=100, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_max_distance_number(n))

        if min_distance_config := controls_config.get(CONF_MIN_DISTANCE):
            n = await number.new_number(min_distance_config, min_value=0, max_value=100, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_min_distance_number(n))

        if min_speed_config := controls_config.get(CONF_MIN_SPEED):
            n = await number.new_number(min_speed_config, min_value=0, max_value=120, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_min_speed_number(n))

        if no_target_delay_config := controls_config.get(CONF_NO_TARGET_DELAY):
            n = await number.new_number(no_target_delay_config, min_value=0, max_value=255, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_no_target_delay_number(n))

        if trigger_count_config := controls_config.get(CONF_TRIGGER_COUNT):
            n = await number.new_number(trigger_count_config, min_value=1, max_value=10, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_trigger_count_number(n))

        if min_snr_config := controls_config.get(CONF_MIN_SNR):
            n = await number.new_number(min_snr_config, min_value=0, max_value=8, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_min_snr_number(n))

        if app_snr_config := controls_config.get(CONF_APP_SNR_THRESHOLD):
            n = await number.new_number(app_snr_config, min_value=0, max_value=64, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_app_snr_threshold_number(n))

        if speed_correction_config := controls_config.get(CONF_SPEED_CORRECTION):
            n = await number.new_number(speed_correction_config, min_value=0.1, max_value=4.0, step=0.01)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_speed_correction_number(n))

        if detection_direction_config := controls_config.get(CONF_DETECTION_DIRECTION):
            s = await select.new_select(detection_direction_config, options=["away", "approach", "both"])
            await cg.register_parented(s, config[CONF_ID])
            cg.add(var.set_detection_direction_select(s))

    if CONF_TARGET_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_TARGET_COUNT])
        cg.add(var.set_target_count_sensor(sens))
    if CONF_ALARM in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_ALARM])
        cg.add(var.set_alarm_binary_sensor(bs))
    if CONF_ANGLE in config:
        sens = await sensor.new_sensor(config[CONF_ANGLE])
        cg.add(var.set_angle_sensor(sens))
    if CONF_DISTANCE in config:
        sens = await sensor.new_sensor(config[CONF_DISTANCE])
        cg.add(var.set_distance_sensor(sens))
    if CONF_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_SPEED])
        cg.add(var.set_speed_sensor(sens))
    if CONF_SNR in config:
        sens = await sensor.new_sensor(config[CONF_SNR])
        cg.add(var.set_snr_sensor(sens))
    if CONF_DIRECTION in config:
        ts = await text_sensor.new_text_sensor(config[CONF_DIRECTION])
        cg.add(var.set_direction_text_sensor(ts))
