# Overview

This sample application provides an example usage of the RTC RV8803 from Microcrystal AG.

- It print the battery flags to check for RTC battery status.
- It sets the RTC time to the `Wed Dec 31 2025 23:59:55 GMT+0000`
- It sets an alarm to send an interrupt each time the RTC time reaches the minute `01` (i.e. each hour at minute `01`).
- Use the alarm callback to change the `clock_OUT` rate between `32.768 kHz` and `1024 Hz`.
- Use the update callback to print a message.
- It gets the RTC time and prints it each second.

> [!NOTE]
> Alarms can not trigger more often than every minute.

# Requirements

- Power Supply: RTC components usually require an additional battery cell.
- Board allowing I2C communication.
- Available IRQ GPIO for Alarm and Update interrupts.
- `CONFIG_RTC=y` in prj.conf to use RTC API.
- Set `CONFIG_RV8803_RTC_ENABLE=n` in prj.conf to disable RTC regardless of `CONFIG_RTC`.
- `CONFIG_RTC_ALARM=y` in prj.conf to use RTC arlams.
- `CONFIG_RTC_UPDATE=y` in prj.conf to use RTC update.
- `CONFIG_CLOCK_CONTROL=y` in prj.conf to use CLK API.
- Set `CONFIG_RV8803_CLK_ENABLE=n` in prj.conf to disable CLK regardless of `CONFIG_CLOCK_CONTROL`.

# References

- [RV-8803-C7 Component](https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-8803-C7.pdf).
- [RV-8803-C7 Datasheet](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8803-C7_App-Manual.pdf).

# Building and Running

```shell
cd <driver_directory>
west build -p always -b <BOARD> samples/
west flash
```

# Sample Output

```shell
*** Booting Zephyr OS build v3.7.0 ***
RV8803 device is ready
RV8803: POR[0] LOW[0]
RTC device is ready
CLK device is ready
Clock rate[0]
RTC set time succeed
RTC get time succeed
Setter[2] datetime [0|0 0:1]
Getter[2] datetime [0|0 0:1]
RTC_TIME[0] [Wed Dec 31 23:59:56 2025]
RTC Update detected!!
RTC_TIME[0] [Wed Dec 31 23:59:57 2025]
RTC Update detected!!
RTC_TIME[0] [Wed Dec 31 23:59:58 2025]
RTC Update detected!!
RTC_TIME[0] [Wed Dec 31 23:59:59 2025]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:00:00 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:00:01 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:00:02 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:00:03 2026]
RTC Update detected!!

...

RTC_TIME[0] [Thu Jan  1 00:00:58 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:00:59 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:01:00 2026]
RTC Update detected!!
RTC Alarm detected: set rate[1024 Hz]!!
RTC_TIME[0] [Thu Jan  1 00:01:01 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:01:02 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 00:01:03 2026]
RTC Update detected!!

...

RTC_TIME[0] [Thu Jan  1 01:00:58 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 01:00:59 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 01:01:00 2026]
RTC Update detected!!
RTC Alarm detected: set rate[32768 Hz]!!
RTC_TIME[0] [Thu Jan  1 01:01:01 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 01:01:02 2026]
RTC Update detected!!
RTC_TIME[0] [Thu Jan  1 01:01:03 2026]
RTC Update detected!!

<repeats endlessly>
```
