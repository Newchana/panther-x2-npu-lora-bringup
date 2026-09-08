#!/bin/bash
# Panther X2 LED 行为修正（开机自启）
# - led-eth : 跟随网卡链路（插网线亮，拔网线灭）
# - led-wifi: 跟随 WiFi 连接（连上 AP 亮，断开灭）
# - led-status: heartbeat（系统正常规律呼吸）
# - led-pwr : 常亮

# 等待网络就绪
for i in $(seq 1 30); do
  [ -e /sys/class/leds/led-eth ] && break
  sleep 1
done

# 网口灯：netdev 触发器，跟随 eth0 链路
if [ -d /sys/class/net/eth0 ]; then
  echo netdev > /sys/class/leds/led-eth/trigger
  echo eth0 > /sys/class/leds/led-eth/device_name 2>/dev/null
  echo 1 > /sys/class/leds/led-eth/link
fi

# WiFi 灯：netdev 触发器，跟随 wlan0 链路（关联 AP 才亮）
if [ -d /sys/class/net/wlan0 ]; then
  echo netdev > /sys/class/leds/led-wifi/trigger
  echo wlan0 > /sys/class/leds/led-wifi/device_name 2>/dev/null
  echo 1 > /sys/class/leds/led-wifi/link
else
  # 无 wlan0 接口时熄灭，不强亮
  echo none > /sys/class/leds/led-wifi/trigger
  echo 0 > /sys/class/leds/led-wifi/brightness
fi

# 状态灯：heartbeat（若未设置）
echo heartbeat > /sys/class/leds/led-status/trigger

# 电源灯常亮
echo 1 > /sys/class/leds/led-pwr/brightness

exit 0
