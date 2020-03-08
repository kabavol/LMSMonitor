# LMSMonitor
OLED information display control program for [piCorePlayer](https://www.picoreplayer.org/) or other Raspberry Pi and Logitech Media Player (LMS) based audio device.

<img width="800" src="doc/IMG_1442.jpg" align="center" />

### Options
```bash
-n Player Name
-c display clock when not playing
-t enable print info to stdout
-l increment verbose level
-v activate visualization
-m VU meters and spectrum analysis
-r display remaining time rather than track time
```

### Features
- Removed static library usage, smaller size, upgrade hardened
- Removed use of ALSA MIMO, audio attributes provided by LMS used
- Track details are displayed only when playing
- Display features independant scrolling of track details.
- Remaining time can now be displayed
- Audio attributes, volume, sample depth, and sample rate are displayed
- A retro clock is displayed when audio paused/stopped.

### Screen-snaps
The following images were captured by dumping the display on refresh; it's a tad blocky but the actual screen animations are buttery smot with a 15 FPS attained.

At 15 FPS scrolling text is smooth and the visualizer modes very kinetic.

<img width="300" src="source/demo.gif" align="center" />

### Visualizer Modes

Currently two visualizer modes are supported
- Stereo VU Meters - dBfs metered
- Stereo 12-band Spectrum Analysis

### Coming soon
- DONE! Audio visualizer support: stereo VU meters
- DONE! Audio visualizer support: spectrum analyzer
- Audio visualizer support: horizontal Peak RMS
- Weather: TBD
- Dual OLED visualizer mode: TBD
- Color 128 x 128 color TFT support