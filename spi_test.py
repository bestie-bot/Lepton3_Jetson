import spidev
import time
spi = spidev.SpiDev()
spi.open(0, 1)  # bus 0, device 0
spi.max_speed_hz = 500000
spi.mode = 0
msg = [0xFF, 0xAA, 0x55, 0x00]
while True:
    resp = spi.xfer2(msg)
    if resp == msg:
        print("SPI loopback test passed")
    else:
        print("SPI loopback test failed")
    # Sleep for a bit
    time.sleep(1)