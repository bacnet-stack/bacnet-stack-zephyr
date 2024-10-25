.. _b-ss_sample:

BACnet Profile B-SS Sample
##########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet Smart Sensor (B-SS) device profile.

Requirements
************

* A board with Ethernet support, for instance: nucleo_f429zi

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-ss` in
the BACnet tree.

The sample can be built for several platforms.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ss/
