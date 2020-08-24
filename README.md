# Supercapacitor based UPS boards
## Scott Baker, http://www.smbaker.com/

# Purpose

These boards are supercapacitor-based uninterruptable power supplies. The supercapacitor charges when power is applied, then when the power supply is terminated, the supercapacitor will operate the device long enough for it to shut down safely.

# Design - 12V board (5v output)

The 12V board uses five 2.7V supercapacitors in series, with 2.7V shunt regulators to ensure that no single capacitor is charged to more than 2.5 volts. A 3W resistor is used to limit the current that charges the capacitors. A jumper is provided to allow the capacitors to be rapid discharged. A mosfet is added to allow the load to be turned on/off.

A murata DC-DC converter regulates the 12V down to 5V 

# Design - 5V board

The 5V board design is similar to the 12V board with the notable difference that only two supercapacitors are used. Instead of the murata DC-DC converter, a Pololu buck/boost module is used that is capable of boosting as low as 3V up to 5V. The Mosfet used in the 12V board is not necessary, we can turn the pololu DC/DC converter on/off directly.

# Microcontroller manager

An ATTiny85 is used as the controller for the UPS. The microcontroller monitors the battery voltage and the incoming voltage, and switches the load on and off. The microcontroller supports I2C and can be interfaced directly to a raspberry pi.

The microcontroller is also responsible for noticing when power has been restored, and turning the load back on.

# Linux Daemon

A Linux daemon, running on the raspberry pi, monitors the voltage by making i2c queries to the ups board's microcontroller. If the daemon sees the incoming voltage drop, then it will initiate shutdown, by running the `shutdown` command. The shutdown command will terminate processes, unmount the filesystem, etc. 

As a final step during shutdown, Linux systemd will execute a script that will tell the microcontroller to turn off the load. At this point, the root filesystem has been remounted readonly, so the board can safely be powered off. 

# Acknowledgements

* David Gesswein's MFM Emulator was the inspiration for the supercapacitor charing circuit.
