# What is DCC++ EX?
DCC++ EX is the organization maintaining several codebases that together represent a fully open source DCC system. Currently, this includes the following:

* [CommandStation-EX](https://github.com/DCC-EX/CommandStation-EX/releases) - the latest take on the DCC++ command station for controlling your trains. Runs on an Arduino board, and includes advanced features such as a WiThrottle server implementation, turnout operation, general purpose inputs and outputs (I/O), and JMRI integration.
* [exWebThrottle](https://github.com/DCC-EX/exWebThrottle) - a simple web based controller for your DCC++ command station.
* [BaseStation-installer](https://github.com/DCC-EX/BaseStation-Installer) - an installer executable that takes care of downloading and installing DCC++ firmware onto your hardware setup.
* [BaseStation-Classic](https://github.com/DCC-EX/BaseStation-Classic) - the original DCC++ software, packaged in a stable release. No active development, bug fixes only.

A basic DCC++ EX hardware setup can use easy to find, widely avalable Arduino boards that you can assemble yourself.

Both CommandStation-EX and BaseStation-Classic support much of the NMRA Digital Command Control (DCC) [standards](http://www.nmra.org/dcc-working-group "NMRA DCC Working Group"), including:

* simultaneous control of multiple locomotives
* 2-byte and 4-byte locomotive addressing
* 128-step speed throttling
* Activate/de-activate all accessory function addresses 0-2048
* Control of all cab functions F0-F28
* Main Track: Write configuration variable bytes and set/clear specific configuration variable (CV) bits (aka Programming on Main or POM)
* Programming Track: Same as the main track with the addition of reading configuration variable bytes

# What’s in this Repository?

This repository, CommandStation-EX, contains a complete DCC++ EX Commmand Station sketch designed for compiling and uploading into an Arduino Uno, Mega, or Nano.  All sketch files are in the folder named CommandStation-EX and its subforlders. 

To utilize this sketch, you can use the following: 

1. (beginner) our [automated installer](https://github.com/DCC-EX/BaseStation-Installer)
2. (intermediate) download the latest version from the [releases page](https://github.com/DCC-EX/CommandStation-EX/releases)
3. (advanced) use git clone on this repository 

Not using the installer? Open the file "CommandStation-EX.ino" in the
Arduino IDE. Please do not rename the folder containing the sketch
code, nor add any files in that folder. The Arduino IDE relies on the
structure and name of the folder to properly display and compile the
code. Rename or copy config.example.h to config.h. If you do not have 
the standard setup, you must edit config.h according to the help texts 
in config.h.

## What's new in CommandStation-EX?

* WiThrottle server built in. Connect Engine Driver or WiThrottle clients directly to your Command Station
* WiFi and Ethernet shield support
* No more jumpers or soldering!
* Direct support for all the most popular motor control boards
* I2C Display support
* Improved short circuit detection and automatic reset from an overload
* Current reading, sensing and ACK detection settings in milliAmps instead of just pin readings
* Improved adherence to the NMRA DCC specification
* Complete support for all the old commands and front ends like JMRI
* Railcom cutout (beta)
* Simpler, modular, faster code with an API Library for developers for easy expansion
* New features and functions in JMRI
* Automation (coming soon)

NOTE: DCC-EX is a major rewrite to the code. We started over and rebuilt it from the ground up! For what that means to you, click [HERE](notes/rewrite.md).

# More information
You can learn more at the [DCC++ EX website](https://dcc-ex.com/)

- November 14, 2020

---

# Building and flashing for the Adafruit Feather M0 (samd_zero fork)

This fork adds a ``[env:feather-m0]`` PlatformIO environment targeting the
Adafruit Feather M0 (SAMD21, 48 MHz, 256 KB flash) with WiFi101, a 20x4 I2C LCD,
an external I2C EEPROM, and the catseye pushbutton UI.

The headers ``debounce.h``, ``periodic_trigger.h`` and ``smooth_on_off.h`` are
vendored under ``lib/pg_sigs/src`` (a minimal subset of
https://github.com/pgriffiths/pg_signals_sytems). ``debounce.h`` here has the
two setter signatures uncommented so it compiles; upstream HEAD has them
commented out.

## 1. Install PlatformIO

Requires Python 3.9+ (``python`` / ``python3``):

```sh
pip install platformio
```

Verify:

```sh
pio --version
```

If ``pio`` is not on PATH, use ``python -m platformio`` in place of ``pio``
below (or add ``<python>/Scripts`` / ``~/.local/bin`` to PATH).

## 2. Configure WiFi credentials

The sketch includes ``arduino_secrets.h`` (gitignored). Create it:

```sh
cp arduino_secrets.example.h arduino_secrets.h
# Windows: copy arduino_secrets.example.h arduino_secrets.h
```

then edit it and set ``SECRET_SSID`` / ``SECRET_PASS``.

## 3. Build

```sh
pio run
```

Produces ``.pio/build/feather-m0/firmware.bin``.

## 4. Find the bossac upload tool

PlatformIO installs bossac into its packages directory:

- Windows: ``%USERPROFILE%\.platformio\packages\tool-bossac\bossac.exe``
- macOS:   ``~/.platformio/packages/tool-bossac/bossac``
- Linux:   ``~/.platformio/packages/tool-bossac/bossac``

Alternatively install BOSSA (https://github.com/shumatech/BOSSA) from your OS
package manager. Note that ``pio run -t upload`` also works when the board is
already in bootloader mode (see next step).

## 5. Put the board in bootloader mode

Double-tap the RESET button quickly. The red LED starts to pulse/"breathe" and
a *new* serial port appears - it is NOT the port the sketch normally uses.
On Windows check Device Manager or:

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

On Linux/macOS: ``ls /dev/ttyACM* /dev/tty.usbmodem*`` or ``pio device list``.
The bootloader port only exists while the bootloader runs; if your earlier
flash left a broken image, the bootloader is still always reachable this way.

## 6. Flash

```sh
bossac -p <PORT> -U true -i -e -w -v .pio/build/feather-m0/firmware.bin
```

- Windows: ``-p COM4`` ; Linux: ``-p /dev/ttyACM0`` ; macOS: ``-p /dev/tty.usbmodemXXXX``
- This bossac (1.7.0) has no ``-o``/``--offset`` option - it automatically
  writes starting at 0x2000, above the 8 KB bootloader, which is never touched.
- ``-e`` erases the app area, ``-w`` writes, ``-v`` verifies, ``-U true`` forces
  USB-port detection, ``-i`` prints device info.

Press RESET once afterwards to start the firmware.

## 7. Read back / verify (optional)

```sh
bossac -p <PORT> -U true -r readback.bin
```

Reads the whole application area (253952 bytes). The first ``firmware.bin``
length bytes should match ``firmware.bin``; the rest is 0xFF apart from up to
63 bytes of page padding.

## 8. Monitor serial output

After reset, find the sketch's COM port (different from the bootloader port)
and run:

```sh
pio device monitor -p <PORT> -b 115200
```

The ``<iDCC-EX V-...>`` banner and ``DCC Init...`` lines appear at startup.

## Known-good versions (tested 2026-11)

- PlatformIO Core 6.2.0, platform ``atmelsam`` 9.0.0
- framework-arduino-samd-adafruit 1.7.16, toolchain gcc 9.3.1
- Libraries: WiFi101 0.16.1, LiquidCrystal_I2C 1.1.4,
  SparkFun External EEPROM 3.2.13
- Firmware size ~76.7 KB (29% of flash), RAM ~12.2 KB

Note: the image historically running on this hardware was built with the
Arduino IDE + Adafruit SAMD core 1.6.5, so it is not byte-identical to a
PlatformIO build, but all application strings match.
