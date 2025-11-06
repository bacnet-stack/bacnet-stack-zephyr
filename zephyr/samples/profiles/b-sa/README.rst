.. _b-sa_sample:

BACnet Profile B-SA Sample
##########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet Smart Actuator (B-SA) device profile.

Requirements
************

* A board with Ethernet support, for instance: nucleo_f429zi

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-sa` in
the BACnet tree.

The sample can be built for several platforms. Use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-sa/

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-sa/

Compile this sample for the `adafruit_grand_central_m4_express` board with

    west build -b adafruit_grand_central_m4_express -p always bacnet/zephyr/samples/profiles/b-sa/

Using the Shell
***************

The shell is available on some boards via virtual communication port:

    picocom --baud 115200 /dev/ttyACM0

    Terminal ready
    *** Booting Zephyr OS build v3.7.0 ***
    [00:00:00.012,000] <inf> bacnet: BACnet Device: BACnet Smart Actuator (B-SA)
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Version 1.4.1
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Max APDU: 1476
    uart:~$
    bacnet   clear    device   devmem   help     history  kernel   net
    rem      resize   retval   shell    stats
    uart:~$ bacnet objects
    {"object-list": [
    {"object-identifier":{"device":260124}},
    {"object-identifier":{"analog-output":1}},
    {"object-identifier":{"network-port":0}}],
    "object-list-size": 3}
