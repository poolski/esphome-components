import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, number, select, sensor, text_sensor, uart
from esphome.const import CONF_ID, DEVICE_CLASS_DISTANCE, DEVICE_CLASS_MOTION, ENTITY_CATEGORY_CONFIG

CODEOWNERS = ["@poolski"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "number", "select"]
MULTI_CONF = True

CONF_TARGET_COUNT = "target_count"
CONF_VEHICLE_DETECTED = "vehicle_detected"
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
CONF_SNR_THRESHOLD = "snr_threshold"
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
LD2451SnrThresholdNumber = ld2451_ns.class_(
    "LD2451SnrThresholdNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451SpeedCorrectionNumber = ld2451_ns.class_(
    "LD2451SpeedCorrectionNumber", number.Number, cg.Parented.template(LD2451Component)
)
LD2451DetectionDirectionSelect = ld2451_ns.class_(
    "LD2451DetectionDirectionSelect", select.Select, cg.Parented.template(LD2451Component)
)
cg.add_global(ld2451_ns.using)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LD2451Component),
            cv.Optional(CONF_MAX_DISTANCE): cv.invalid(
                f"'{CONF_MAX_DISTANCE}' has been moved to the 'controls: {CONF_MAX_DISTANCE}' number entity"
            ),
            cv.Optional(CONF_MIN_DISTANCE): cv.invalid(
                f"'{CONF_MIN_DISTANCE}' has been moved to the 'controls: {CONF_MIN_DISTANCE}' number entity"
            ),
            cv.Optional(CONF_MIN_SPEED): cv.invalid(
                f"'{CONF_MIN_SPEED}' has been moved to the 'controls: {CONF_MIN_SPEED}' number entity"
            ),
            cv.Optional(CONF_DETECTION_DIRECTION): cv.invalid(
                f"'{CONF_DETECTION_DIRECTION}' has been moved to the 'controls: {CONF_DETECTION_DIRECTION}' select entity"
            ),
            cv.Optional(CONF_NO_TARGET_DELAY): cv.invalid(
                f"'{CONF_NO_TARGET_DELAY}' has been moved to the 'controls: {CONF_NO_TARGET_DELAY}' number entity"
            ),
            cv.Optional(CONF_TRIGGER_COUNT): cv.invalid(
                f"'{CONF_TRIGGER_COUNT}' has been moved to the 'controls: {CONF_TRIGGER_COUNT}' number entity"
            ),
            cv.Optional(CONF_MIN_SNR): cv.invalid(
                f"'{CONF_MIN_SNR}' has been moved to the 'controls: {CONF_MIN_SNR}' number entity"
            ),
            cv.Optional(CONF_SPEED_CORRECTION): cv.invalid(
                f"'{CONF_SPEED_CORRECTION}' has been moved to the 'controls: {CONF_SPEED_CORRECTION}' number entity"
            ),
            cv.Optional(CONF_CONTROLS): cv.Schema(
                {
                    # Max detection distance in metres (10..100). Targets beyond this are ignored by the device.
                    cv.Optional(CONF_MAX_DISTANCE): number.number_schema(
                        LD2451MaxDistanceNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:ruler",
                    ),
                    # Min detection distance in metres (0..100). Software-only filter; does not affect the device.
                    cv.Optional(CONF_MIN_DISTANCE): number.number_schema(
                        LD2451MinDistanceNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:ruler-square",
                    ),
                    # Minimum target speed in km/h (0..120). Targets slower than this are not reported by the device.
                    cv.Optional(CONF_MIN_SPEED): number.number_schema(
                        LD2451MinSpeedNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:speedometer",
                    ),
                    # Seconds (0..255) after the last detection before the device stops reporting the target.
                    cv.Optional(CONF_NO_TARGET_DELAY): number.number_schema(
                        LD2451NoTargetDelayNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:timer-outline",
                    ),
                    # Consecutive detections (1..10) required before the device reports a target. Higher = less false triggers.
                    cv.Optional(CONF_TRIGGER_COUNT): number.number_schema(
                        LD2451TriggerCountNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:counter",
                    ),
                    # Native SNR threshold level: 0 = device default (4); 3..8 — higher value = lower sensitivity.
                    cv.Optional(CONF_MIN_SNR): number.number_schema(
                        LD2451MinSnrNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:signal",
                    ),
                    # App-scale SNR threshold (0..64) mapped to native levels (0, 3..8). Alternative to min_snr.
                    cv.Optional(CONF_SNR_THRESHOLD): number.number_schema(
                        LD2451SnrThresholdNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:signal-distance-variant",
                    ),
                    # Multiplier applied to published speed (0.1..4.0). Software-only; does not affect the device.
                    cv.Optional(CONF_SPEED_CORRECTION): number.number_schema(
                        LD2451SpeedCorrectionNumber,
                        entity_category=ENTITY_CATEGORY_CONFIG,
                        icon="mdi:tune-variant",
                    ),
                    # Detection direction filter: away = opposite-direction only; approach = same-direction only; both = all.
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
            cv.Optional(CONF_VEHICLE_DETECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOTION,
                icon="mdi:car",
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
    .extend(cv.COMPONENT_SCHEMA)
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

        if snr_threshold_config := controls_config.get(CONF_SNR_THRESHOLD):
            n = await number.new_number(snr_threshold_config, min_value=0, max_value=64, step=1)
            await cg.register_parented(n, config[CONF_ID])
            cg.add(var.set_snr_threshold_number(n))

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
    if CONF_VEHICLE_DETECTED in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_VEHICLE_DETECTED])
        cg.add(var.set_vehicle_detected_binary_sensor(bs))
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
