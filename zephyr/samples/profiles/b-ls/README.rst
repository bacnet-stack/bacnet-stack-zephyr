.. _b-ss_sample:

BACnet Profile B-LS Sample
##########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet Lighting Supervisor (B-LS) device profile.

L.11.1 BACnet Lighting Supervisor (B-LS)
****************************************

A B-LS is any device that implements Channel objects and optionally
Lighting Output and/or Binary Lighting Output objects
with the ability to forward channel writes to other BACnet devices.

Data Sharing
************

* Ability to provide values for any of its BACnet objects upon request
* Ability to allow modification of some or all of its BACnet objects
  by another device
* Ability to execute WriteGroup commands
* Ability to propagate Channel values to objects external to the dev

Device and Network Management
*****************************

* Ability to find other BACnet devices
* Ability to respond to queries about its status
* Ability to respond to requests for information about any of its objects
* Ability to respond to communication control messages
* Ability to synchronize its internal clock upon request

Requirements
************

* A board with Ethernet support, for instance: nucleo_f429zi

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-ls` in
the BACnet tree.

The sample can be built for several platforms - use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ls/

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-ls/

Compile this sample for the `adafruit_grand_central_m4_express` board with

    west build -b adafruit_grand_central_m4_express -p always bacnet/zephyr/samples/profiles/b-ls/
