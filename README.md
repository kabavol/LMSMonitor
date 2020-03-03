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

### Coming soon
- Audio visualizer support: stereo VU meters
- Audio visualizer support: spectrum analyzer
- Weather: TBD
- Dual OLED VU meter mode: TBD
