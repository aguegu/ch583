@aguegu/ch583
===

This repo is folked from https://github.com/openwch/ch583. The goal is provide a quick and plain work environment to ch58x development under Linux and without `Mounriver Studio`.

1. Setup Toolchains

* GCC toolchains

*riscv-embedded-gcc.tar.xz* is the gzipped of 'RISC-V Embedded GCC' folder in [MRS_Toolchain_Linux_x64_V1.80.tar.xz](http://file.mounriver.com/tools/MRS_Toolchain_Linux_x64_V1.80.tar.xz), extract it with

```
tar xJf riscv-embedded-gcc.tar.xz`
```

* install USB driver for ISP

*isp/WCHISPTool_CMD* folder comes from [WCHISPTool_CMD_ZIP](https://www.wch.cn/downloads/WCHISPTool_CMD_ZIP.html) (Linux x64)
*isp/config.ini* is exported from [WCHISPTool on Windows](https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html)

```
cd isp/driver
sudo make install
```

2. Try Demo Codes

* Goto `examples`, like `examples/UART1`

```
cd examples/UART1
make
```

It would get the *hex* file compiled in its `dist/` folder.

To program this hex into the board, you would have to make the chip go into ISP mode.

Take the official `CH583M-R0-1v1` EVT board [Circuit schematic](EVT/PUB/CH583SCH.PDF) as example, it would take these steps.

![CH583 EVT Board](datasheets/ch583-evt.avif)

  1. Connect the onboard USB-C port to the PC with a regular USB-A to USB-C cable.

  2. Press and hold the onboard `Download` button down.

  3. Turn the onboard Power Switch from `OFF` to `ON`, you would see the onboard RED power LED turned on in this process. Then the `Download` button could be released.

      * If the hardware connection is correct, a new usb device should show up with `lsusb` command.
      ```
      Bus XXX Device YYY: ID 4348:55e0 WinChipHead
      ```

      * If `isp/driver` is installed successfully, a new device should show up with `ls -l/dev/ch37*` command.
      ```
      crw------- 1 root root 180, 200 Nov  1 16:24 /dev/ch37x0
      ```

      ***But the MCU would exit ISP mode in like 5 seconds***

  4. During this short ISP period, try

  ```
  make flash
  ```

  If everything works, the output would look like

  ```
  sudo ../../isp/WCHISPTool_CMD -p /dev/ch37x0 -c ../../isp/config.ini -o program -f dist/UART1.hex

  =====ISP_Command_Tool=====

  TOOL VERSION:  V3.60

  p:/dev/ch37x0
  b:0
  v:0
  c:../../isp/config.ini
  o:0
  f:dist/UART1.hex

  {"Device":"/dev/ch37x0","Status":"Ready"}
  {"Device":"/dev/ch37x0", "Status":"Programming", "Progress":100%}
  {"Device":/dev/ch37x0", "Status":"Finished", "Code":0,"Message":"Succeed"}
  ```

Enjoy and have fun.

---
# RISC-V Core BLE5.3 MCU, CH583

### Overview

The CH583 is a 32-bit RISC microcontroller integrated with BLE wireless communication. The CH583 provides abundant peripheral sources, such as 2Mbps BLE communication module, 2 full-speed USB host and device controller and transceiver, 2 SPIs, 4 UARTs, ADC, touch-key detection module and RTC, etc.

### Features

- 32-bit RISC processor, WCH RISC-V4A

- RV32IMAC instruction sets, hardware multiplication and division

- 32KB SRAM, 1MB Flash. ICP, ISP and IAP. OTA wireless upgrade

- B uilt-in 2.4GHz RF transceiver and band and link control. BLE5.3

- 2Mbps, 1Mbps, 500Kbps and 125Kbps

- -98dBm receiving sensitivity, programmable +7dBm transfer power

- Protocol stack and API

- Built-in temperature sensor

- Built-in RTC, supports two modes, timing and trigger

- TwoUSB2.0 full-speed Host/Devices

- 14-channel touch-key

- 14-channel 12-bit ADC

- 4UARTs, 2SPIs, 12-channel PWM, and 1-channel IIC

- 40GPIO ports, of which 4 ports support 5V signal input

- Minimum power supply of 1.7V

- Built-in AES-128 encryption/decryption unit, unique chip ID

- Package: QFN48
