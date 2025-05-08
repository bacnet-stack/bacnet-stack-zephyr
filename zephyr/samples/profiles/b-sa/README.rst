.. _b-ss_sample:

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
