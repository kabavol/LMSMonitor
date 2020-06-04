# LMSMonitor
OLED information display control program for [piCorePlayer](https://www.picoreplayer.org/) or other Raspberry Pi and Logitech Media Player (LMS) based audio device.

<img width="800" src="doc/IMG_1442.jpg" align="center" />

### Options
```bash
Usage -n "player name" [options]

options:
 -a all-in-one. One screen to rule them all. Track and visualizer on one screen (pi only)
 -b automatically set brightness of display at sunset and sunrise (pi only)
 -c display clock when not playing (Pi only)
 -d downmix audio and display a single large meter, SA and VU only
 -f font used by clock, see list below for details
 -i increment verbose level
 -k show CPU load and temperature (clock mode)
 -m if visualization on specify one or more meter modes, sa, vu, pk, st, or rn for random
 -o specifies OLED "driver" type (see options below)
 -r show remaining time rather than track time
 -S scrollermode: 0 (cylon), 1 (infinity left), 2 infinity (right)
 -v enable visualization sequence when playing (Pi only)
 -x specifies OLED address if default does not work - use i2cdetect to find address (Pi only)
 -z no splash screen

Supported OLED types:
    1 ...: Adafruit SPI 128x64
    3 ...: Adafruit I2C 128x64
    4 ...: Seeed I2C 128x64
    6* ..: SH1106 I2C 128x64
    7 ...: SH1106 SPI 128x64

* is default

OLED Clock Fonts:
    0 ...: Classic LCD Clock Font
    1 ...: Deco-Solid Font
    2 ...: Deco-Hollow Font
    3 ...: LCD 25x44
    4 ...: Festus Hollow 25x44
    5 ...: Festus Solid 25x44
    6 ...: Space 1999
    7 ...: Roboto Thin
    8 ...: noto 25x44
    9 ...: noto fancy 25x44
   10 ...: Colby Typo 25x44
   11 ...: TTY Pongo 25x44
   12 ...: Windswept 3D 25x44

```

### Features
- Removed static library usage, smaller size, upgrade hardened
- Removed use of ALSA MIMO, audio attributes provided by LMS are used
- Track details are displayed only when playing
- Display features independant scrolling of track details.
- Remaining time can now be displayed rather than total time
- Audio attributes, volume, sample depth, and sample rate are shown
- A retro clock is displayed when the audio paused/stopped.
- Automatically sets the brightness of the display at dawn and dusk.
- Multiple audio visualization modes are supported

### Screen-snaps
The following images were captured by dumping the display on refresh; it's a tad blocky but the actual screen animations are buttery smooth.

See the videos in the base folder, scrolling text is smooth and the visualizer modes very kinetic.

<img width="300" src="base/demo.gif" align="center" />

These font examples were captured using an OLED on a flatbed scanner, the final images being tweaked for contrast.
Fonts examples are 0, at 14:04, through 12, at 13:10. 

<img width="300" src="base/fonts.gif" align="center" />

### Visualizer Modes

Six visualizer modes are supported
- Stereo VU Meters - dBfs metered
- Stereo 12-band Spectrum Analysis
- Stereo 12-band "tornado" Spectrum Analysis
- Stereo 12-band "mirror" Spectrum Analysis
- Stereo Peak Meter - dBfs metered
- Large Downmix (visual data only) VU meter
- Large Downmix (visual data only) Spectrum

### Installation

There are two modes of operation (3 if you include the text only mode)

- LMSMonitor installed on piCore Player, consuming visualization data directly
- LMSMonitor installed on an alternate device, the LMS Server for example, consuming streamed visualization data

# Prerequisites

If you are intending to consume visualization data you need to configure squeezelite to expose the shared memory

From the Squeezlite page of the pCP web frontend type 1 in the "m" ALSA parameter section

And, in the Various Options add *-v*

See the squeezelite page for more details

We also need to install the i2c tools library so we can review setup and communicate with the OLED screen

From the main web form click on Extensions button in the *Additional functions* section

On the page displayed select *i2c-tools-dev.tcz* from the dropdown and install

## pCP Install

SSH to your pCP device.

cd to the /mnt/mmcblk0p2/tce folder

and, then type:

```bash
wget "https://github.com/shunte88/LMSMonitor/blob/master/bin/lmsmonitorpcp.tgz?raw=true" -O lmsmonitorpcp.tgz && tar -xzvf lmsmonitorpcp.tgz
```

This downloads the monitor archive to pCP and extracts the contents

To ensure smooth running perfform the following:

```bash
chmod +x gomonitor
```

With that you can manually start the monitor specifying the visualization you'd like to display, vu, sa, pk, st, sm or rn

For example:

```bash
./gomonitor rn
```

You should see the monitor logo screen appear.  You're pretty much done

## Automated start-up

If you'd like the monitor to automatically start with your pCP and squeezelite setup goto the *Tweaks* page of the pCP web forms.

Add a *User command*, here for example requesting the random visualizations

```bash
/mnt/mmcblk0p2/tce/gomonitor rn
```

Additional supported commands may also be specified, here we request a specific visualizer sequence, the device driver, override the default OLED address, request downmixed visualizers, and automated display brightness at dawn and dusk

```bash
/mnt/mmcblk0p2/tce/gomonitor vu,sa,pk,st -o 6 -x 0x3c -db
```

the visualization parameter must always be specified first

### Coming soon

- DONE! Audio visualizer support: stereo VU meters
- DONE! Audio visualizer support: spectrum analyzer
- DONE! Audio visualizer support: tornado spectrum analyzer
- DONE! Audio visualizer support: mirror spectrum analyzer
- DONE! Audio visualizer support: horizontal Peak RMS
- DONE! Audio visualizer support: random and multiple meters
- DONE! Set display brightness, day and night modes.
- DONE! Downmix visual data and display on one large VU meter.
- DONE! Downmix visual data and display on one large Spectrum.
- DONE! Make OLED driver user selectable
- DONE! Make OLED I2C address user selectable
- DONE: All-In-One display, clock, track details and downmix visualizer in one 
- Dual OLED visualizer mode: TBD
- 128 x 128 OLED support: TBD
- Color 128 x 128 color TFT support: TBD
- Weather: TBD

## Like The App - Git The Shirt

Team Badger shirts are here <https://www.zazzle.com/team_badger_t_shirt-235604841593837420>

## Credits

OLED interface based on ArduiPI_OLED: <https://github.com/hallard/ArduiPi_OLED>
(which is based on the Adafruit_SSD1306, Adafruit_GFX, and bcm2835 libraries).

C library for Broadcom BCM 2835: <https://www.airspayce.com/mikem/bcm2835/>