# LMSMonitor
OLED information display control program for [piCorePlayer](https://sites.google.com/site/picoreplayer/)

 ![image](https://raw.githubusercontent.com/kabavol/LMSMonitor/master/doc/LMSMonitorV02_owmr.jpg)

### Installation
- Copy the `lmsmonitor` and `startDisp.sh` files to the `/etc/sysconfig/tcedir/` directory.
- Go to your devices web page, select the `normal` or `advanced` mode and go to the `Tweaks` tab.
- Select a `User commands` field and set `/etc/sysconfig/tcedir/startDisp.sh`
- Reboot

# Tested on:
- Raspberry Pi B+
- IQaudIO Pi-DAC+
- 1.3'' 128 x 64 White Color OLED Display Module w/ I2C Interface
