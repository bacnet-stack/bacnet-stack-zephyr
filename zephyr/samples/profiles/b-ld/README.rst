.. _b-ss_sample:

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

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-ld` in
the BACnet tree.

The sample can be built for several platforms - use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ld/

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-ld/
