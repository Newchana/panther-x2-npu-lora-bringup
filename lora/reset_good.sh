#!/bin/sh
SX1302_RESET_PIN=120
SX1302_POWER_EN_PIN=129
WAIT(){ sleep 0.1; }
echo "$SX1302_RESET_PIN" > /sys/class/gpio/export 2>/dev/null
echo "$SX1302_POWER_EN_PIN" > /sys/class/gpio/export 2>/dev/null
echo out > /sys/class/gpio/gpio$SX1302_RESET_PIN/direction 2>/dev/null
echo out > /sys/class/gpio/gpio$SX1302_POWER_EN_PIN/direction 2>/dev/null
echo 1 > /sys/class/gpio/gpio$SX1302_POWER_EN_PIN/value; WAIT
echo 1 > /sys/class/gpio/gpio$SX1302_RESET_PIN/value; WAIT
echo 0 > /sys/class/gpio/gpio$SX1302_RESET_PIN/value; WAIT
exit 0
