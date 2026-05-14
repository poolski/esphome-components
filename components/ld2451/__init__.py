import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, text_sensor, uart
from esphome.const import CONF_ID, DEVICE_CLASS_DISTANCE, DEVICE_CLASS_MOTION

CODEOWNERS = ["@poolski"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

CONF_TARGET_COUNT = "target_count"
CONF_ALARM = "alarm"
CONF_ANGLE = "angle"
CONF_DISTANCE = "distance"
CONF_SPEED = "speed"
CONF_SNR = "snr"
CONF_DIRECTION = "direction"

ld2451_ns = cg.esphome_ns.namespace("ld2451")
LD2451Component = ld2451_ns.class_("LD2451Component", cg.Component, uart.UARTDevice)
cg.add_global(ld2451_ns.using)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LD2451Component),
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
                accuracy_decimals=0,
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
