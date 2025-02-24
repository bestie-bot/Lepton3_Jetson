# Lepton3_Jetson

Library and examples to connect the FLIR Lepton3 thermal camera to Nvidia Jetson Orin Nano (specific) embedded boards

## Prerequisites

* [Flir Lepton 3 or 3.5 module](https://www.flir.it/products/lepton/?model=500-0276-01)
* [Breakout Board v2.0 by Getlab](https://www.flir.com/products/lepton-breakout-board-v2.0?vertical=microcam&segment=oem)
* [An Nvidia Jetson board](https://www.nvidia.com/en-us/autonomous-machines/jetson-store/) (tested on Jetson Orin Nano with Jetpack 5.1.2)
* OpenCV library to compile the examples

## Software Installation

Install build requirements

```
sudo apt install build-essential g++ libopencv-dev libssl-dev python3-dev
```

### Build the project

Clone this repository

```
git clone https://github.com/bestie-bot/Lepton3_Jetson
```
   
Compile

```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ cd ..
```

## Camera connection

Here are the pin configs:

| Breakout Board | Jetson Orin Nano | Connection |
|----------|----------|----------|
| 5   | 27   | SDA |
| 7    | 23   | SPI0_SCK |
| 8    | 28   | SCL |
| 10    | 24   | SPI0_CS |
| 12    | 21   | SPI0_MISO |
| J2 - Top    | Any Ground  |
| J2 - Bottom    | Any Power   |

Older reference instruction about how to connect the previous 1.4 Lepton3 module to a Nvidia Jetson Nano are available on [Myzhar's website](https://www.myzhar.com/blog/?p=4500).

### SPI Fixes and Customization

The default buffer size use for SPI communication is set to 8192 bytes by the spidev module. Lepton3 requires 20KB of buffer to retrieve a full segment of data that composes the thermal image.

Here are some notes on challenges and gotchas:
- Make sure you add your username to the group `gpio` by typing `sudo usermod -a -G gpio nvidia`
- If you're having problems loading `spi` in the `/dev/spi*` section, `sudo modprobe spidev` should load them manually
- To make sure `spi` loads on launch, go here: `cd /etc/modules-load.d/` and edit `modules.conf`. Add `spidev` to the file, save and reboot
- Need to increase the SPIdev buffer size to 20k:
```
sudo nano /etc/modprobe.d/spidev.conf
```
`options spidev bufsize=20480`
You can verify the buffer size here:
`cat /sys/module/spidev/parameters/bufsize`

You must change the size of the SPI buffer for the Jetson Orin Nano. Older instructions may be found on the [Myzhar's blog](https://www.myzhar.com/blog/jetson-nano-with-flir-lepton3/#Change_SPI_buffer_size).

## Run the Demos

Two examples are provided to illustrate how to use the `lepton3_grabber` static library available in the folder `build/grabber_lib`.

### OpenCV Demo

With this sample you can see how to use OpenCV to display the thermal stream and control the camera behaviors

```
$ cd build/opencv_demo
$ ./opencv_demo
```

Keyboard commands:
* `c` -> RGB mode (24bit RGB color images)
* `r` -> Radiometry mode (16 bit gray image containing 14 bit linear thermal values )
* `h` -> High gain mode (-10°C to 140°C with 5°C of accuracy)
* `l` -> Low gain mode ( -10°C to 400°C with 10°C of accuracy)
* `a` -> Auto gain mode
* `f` -> Perform FFC normalization
* `F` -> Perform FFC radiometry normalization

### Fever control demo

With this demo you can see how to estimate temperatures from 16 bit gray images to evaluate the temperature of a person and get alarms in case of fever. 
I created this demo as a security sample application to be used in the COVID19 period.

```
$ cd build/check_fever_app
$ ./check_fever_app
```

Using keyboard `u`/`d` you can increase/decrease the estimated temperature su simulate person fever.

See the demo on [YouTube](https://youtu.be/SFStaq--3-U) 

<img src="images/FeverNormal.png" width="250" height="250"> | <img src="images/FeverWarning.png" width="250" height="250"> | <img src="images/FeverAlert.png" width="250" height="250">

    

