import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, text_sensor, uart
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_MOTION,
    DEVICE_CLASS_SPEED,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL,
    UNIT_KILOMETER_PER_HOUR,
    UNIT_METER,
)

CODEOWNERS = ["@poolski"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

CONF_TARGET_COUNT = "target_count"
CONF_VEHICLE_DETECTED = "vehicle_detected"
CONF_ANGLE = "angle"
CONF_DISTANCE = "distance"
CONF_SPEED = "speed"
CONF_SPEED_MPH = "speed_mph"
CONF_SNR = "snr"
CONF_DIRECTION = "direction"
CONF_SPEED_PUBLISH_MAX_ABS_ANGLE = "speed_publish_max_abs_angle"
CONF_MIN_DISTANCE = "min_distance"
CONF_SPEED_CORRECTION = "speed_correction"
LIVE_TARGET_SLOT_NAMES = ("target_1", "target_2", "target_3")
LIVE_TARGET_STAT_SUFFIXES = ("min", "max", "avg")
LIVE_TARGET_SENSOR_SPECS = (
    (
        "x",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=0,
            icon="mdi:axis-x-arrow",
        ),
        "set_live_target_x_sensor",
    ),
    (
        "y",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=0,
            icon="mdi:axis-y-arrow",
        ),
        "set_live_target_y_sensor",
    ),
    (
        "angle",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement="deg",
            accuracy_decimals=0,
            icon="mdi:angle-obtuse",
        ),
        "set_live_target_angle_sensor",
    ),
    (
        "distance",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DISTANCE,
            icon="mdi:map-marker-distance",
        ),
        "set_live_target_distance_sensor",
    ),
    (
        "speed",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_SPEED,
            icon="mdi:speedometer",
        ),
        "set_live_target_speed_sensor",
    ),
    (
        "speed_mph",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement="mph",
            accuracy_decimals=2,
            icon="mdi:speedometer",
        ),
        "set_live_target_speed_mph_sensor",
    ),
    (
        "snr",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_DECIBEL,
            accuracy_decimals=0,
            icon="mdi:signal",
        ),
        "set_live_target_snr_sensor",
    ),
    (
        "direction",
        lambda: text_sensor.text_sensor_schema(icon="mdi:sign-direction"),
        "set_live_target_direction_text_sensor",
    ),
)
LIVE_TARGET_STAT_SENSOR_SPECS = (
    (
        "distance",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DISTANCE,
            icon="mdi:map-marker-distance",
        ),
    ),
    (
        "speed",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_SPEED,
            icon="mdi:speedometer",
        ),
    ),
    (
        "speed_mph",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement="mph",
            accuracy_decimals=2,
            icon="mdi:speedometer",
        ),
    ),
    (
        "snr",
        lambda: sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            unit_of_measurement=UNIT_DECIBEL,
            accuracy_decimals=0,
            icon="mdi:signal",
        ),
    ),
)
ld2451_ns = cg.esphome_ns.namespace("ld2451")
LD2451Component = ld2451_ns.class_("LD2451Component", cg.Component, uart.UARTDevice)
cg.add_global(ld2451_ns.using)

config_schema = {
    cv.GenerateID(): cv.declare_id(LD2451Component),
    cv.Optional("controls"): cv.invalid(
        "runtime configuration via ESPHome is disabled; configure the LD2451 from the mobile app"
    ),
    cv.Optional(CONF_TARGET_COUNT): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement="targets",
        accuracy_decimals=0,
        icon="mdi:counter",
    ),
    cv.Optional(CONF_VEHICLE_DETECTED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_MOTION,
        icon="mdi:car",
    ),
    cv.Optional(CONF_ANGLE): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement="deg",
        accuracy_decimals=0,
        icon="mdi:angle-obtuse",
    ),
    cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement=UNIT_METER,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_DISTANCE,
        icon="mdi:map-marker-distance",
    ),
    cv.Optional(CONF_SPEED): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_SPEED,
        icon="mdi:speedometer",
    ),
    cv.Optional(CONF_SPEED_MPH): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement="mph",
        accuracy_decimals=2,
        icon="mdi:speedometer",
    ),
    cv.Optional(CONF_SNR): sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement=UNIT_DECIBEL,
        accuracy_decimals=0,
        icon="mdi:signal",
    ),
    cv.Optional(CONF_DIRECTION): text_sensor.text_sensor_schema(
        icon="mdi:sign-direction",
    ),
    cv.Optional(CONF_SPEED_PUBLISH_MAX_ABS_ANGLE, default=0): cv.int_range(
        min=0, max=90
    ),
    cv.Optional(CONF_MIN_DISTANCE, default=0): cv.int_range(min=0, max=100),
    cv.Optional(CONF_SPEED_CORRECTION, default=1.0): cv.positive_float,
}
for slot_name in LIVE_TARGET_SLOT_NAMES:
    for field_name, schema_factory, _ in LIVE_TARGET_SENSOR_SPECS:
        config_schema[cv.Optional(f"{slot_name}_{field_name}")] = schema_factory()
    for field_name, schema_factory in LIVE_TARGET_STAT_SENSOR_SPECS:
        for suffix in LIVE_TARGET_STAT_SUFFIXES:
            config_schema[cv.Optional(f"{slot_name}_{field_name}_{suffix}")] = (
                schema_factory()
            )

CONFIG_SCHEMA = (
    cv.Schema(config_schema).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)
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
    if CONF_SPEED_MPH in config:
        sens = await sensor.new_sensor(config[CONF_SPEED_MPH])
        cg.add(var.set_speed_mph_sensor(sens))
    if CONF_SNR in config:
        sens = await sensor.new_sensor(config[CONF_SNR])
        cg.add(var.set_snr_sensor(sens))
    if CONF_DIRECTION in config:
        ts = await text_sensor.new_text_sensor(config[CONF_DIRECTION])
        cg.add(var.set_direction_text_sensor(ts))
    cg.add(
        var.set_speed_publish_max_abs_angle(config[CONF_SPEED_PUBLISH_MAX_ABS_ANGLE])
    )
    cg.add(var.set_min_distance(config[CONF_MIN_DISTANCE]))
    cg.add(var.set_speed_correction(config[CONF_SPEED_CORRECTION]))
    for slot_index, slot_name in enumerate(LIVE_TARGET_SLOT_NAMES):
        for field_name, _, setter_name in LIVE_TARGET_SENSOR_SPECS:
            key = f"{slot_name}_{field_name}"
            if key not in config:
                continue
            if field_name == "direction":
                slot_sensor = await text_sensor.new_text_sensor(config[key])
            else:
                slot_sensor = await sensor.new_sensor(config[key])
            if setter_name == "set_live_target_x_sensor":
                cg.add(var.set_live_target_x_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_y_sensor":
                cg.add(var.set_live_target_y_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_angle_sensor":
                cg.add(var.set_live_target_angle_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_distance_sensor":
                cg.add(var.set_live_target_distance_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_speed_sensor":
                cg.add(var.set_live_target_speed_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_speed_mph_sensor":
                cg.add(var.set_live_target_speed_mph_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_snr_sensor":
                cg.add(var.set_live_target_snr_sensor(slot_index, slot_sensor))
            elif setter_name == "set_live_target_direction_text_sensor":
                cg.add(
                    var.set_live_target_direction_text_sensor(slot_index, slot_sensor)
                )
        for field_name, _ in LIVE_TARGET_STAT_SENSOR_SPECS:
            for suffix in LIVE_TARGET_STAT_SUFFIXES:
                key = f"{slot_name}_{field_name}_{suffix}"
                if key not in config:
                    continue
                slot_sensor = await sensor.new_sensor(config[key])
                cg.add(
                    getattr(var, f"set_live_target_{field_name}_{suffix}_sensor")(
                        slot_index, slot_sensor
                    )
                )
