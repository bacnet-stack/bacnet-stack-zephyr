.. _b-ld_sample:

BACnet Profile B-LD Sample
##########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet Lighting Device (B-LD) device profile.

L.11.2 BACnet Lighting Device (B-LD)
*************************************

A B-LD is any device that implements Binary Lighting Output and/or Lighting Output objects

Data Sharing
************

* Ability to provide values for any of its BACnet objects upon request
* Ability to allow modification of some or all of its BACnet objects by another device

Device and Network Management
*****************************

* Ability to respond to queries about its status
* Ability to respond to requests for information about any of its objects
* Ability to respond to communication control messages

Requirements
************

* A board with Ethernet support, for instance: nucleo_f429zi

Note: B-LD drives PWM LED hardware when a ``pwm-leds`` devicetree node is
available. On ``native_sim`` (which has no LED hardware), the sample still
runs and logs lighting output changes without attempting hardware LED updates.

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-ld` in
the BACnet tree.

The sample can be built for several platforms - use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ld/
    west flash

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-ld/
    west flash

Compile this sample for the `adafruit_grand_central_m4_express` board with

    west build -b adafruit_grand_central_m4_express -p always bacnet/zephyr/samples/profiles/b-ld/
    west flash

Compile and run this sample for the `native_sim` board:

    west build -t run -b native_sim -p always bacnet/zephyr/samples/profiles/b-ld/

Using the Shell
***************

The shell is available on some boards via virtual communication port:

    picocom --baud 115200 /dev/ttyACM0

    Terminal ready
    *** Booting Zephyr OS build v3.7.0 ***
    [00:00:00.012,000] <inf> bacnet: BACnet Device: BACnet Lighting Device (B-LD)
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Version 1.4.1
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Max APDU: 1476
    uart:~$
    bacnet   clear    device   devmem   help     history  kernel   net
    rem      resize   retval   shell    stats
    uart:~$ bacnet objects
    {"object-list": [
    {"object-identifier":{"device":4194303}},
    {"object-identifier":{"network-port":0}},
    {"object-identifier":{"lighting-output":1}}],
    "object-list-size": 3}
