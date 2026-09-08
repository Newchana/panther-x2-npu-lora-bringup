import fcntl, struct, time, os, sys

I2C_SLAVE = 0x0703
BUS = "/dev/i2c-2"
ADDR = 0x40

def rd(cmd, delay, n=3):
    fd = os.open(BUS, os.O_RDWR)
    try:
        fcntl.ioctl(fd, I2C_SLAVE, ADDR)
        os.write(fd, bytes([cmd]))
        time.sleep(delay)
        data = os.read(fd, n)
    finally:
        os.close(fd)
    return data

t = rd(0xF3, 0.12)
c = (t[0] << 8) | t[1]
temp = -46.85 + 175.72 * c / 65535.0

h = rd(0xF5, 0.12)
c2 = (h[0] << 8) | h[1]
rh = -6.0 + 125.0 * c2 / 65535.0

print("SHT2x raw temp bytes:", t.hex())
print("Temperature: %.2f C" % temp)
print("SHT2x raw rh bytes:", h.hex())
print("Humidity: %.2f %%RH" % rh)
