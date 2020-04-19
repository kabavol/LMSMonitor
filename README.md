I'm sorry, I don't currently have a device that uses a picore player, so I'm not developing this program any further. I ask all those interested, use [shunte88's fork.](https://github.com/shunte88/LMSMonitor)
I would like to thanks to shunte88 for continuing the work!

# LMSMonitor
OLED information display control program for [piCorePlayer](https://sites.google.com/site/picoreplayer/) or other Raspberry Pi and Logitech Media Player (LMS) based audio device.

 ![image](https://raw.githubusercontent.com/kabavol/LMSMonitor/master/doc/LMSMonitorV02_owmr.jpg)

### Options
```bash
-n PlayerName
-o Soundcard (eg. hw:CARD=IQaudIODAC)
-t enable print info to stdout
-v increment verbose level
```

### Installation on piCorePlayer
You can find the precompiled binaries on the [bin folder](https://github.com/kabavol/LMSMonitor/tree/master/bin)

- Copy the `lmsmonitor` and `startDisp.sh` files to the `/etc/sysconfig/tcedir/` directory.
- Go to your devices web page, select the `normal` or `advanced` mode and go to the `Tweaks` tab.
- Select a `User commands` field and set `/etc/sysconfig/tcedir/startDisp.sh`
- Reboot

### Tested on:
- Raspberry Pi A+
- Raspberry Pi B+
- IQaudIO Pi-DAC+
- 1.3" 128 x 64 White Color OLED Display Module with I2C Interface.
