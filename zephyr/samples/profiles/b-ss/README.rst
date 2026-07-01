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

The sample can be built for several platforms - use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ss/
    west flash

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-ss/
    west flash

Compile this sample for the `adafruit_grand_central_m4_express` board with

    west build -b adafruit_grand_central_m4_express -p always bacnet/zephyr/samples/profiles/b-ss/
    west flash

Compile and run this sample for the `native_sim` board:

    west build -t run -b native_sim -p always bacnet/zephyr/samples/profiles/b-ss/

End-to-end ``native_sim`` networking setup flow (Linux host):

Terminal #1 (set up host TAP networking using Zephyr net-tools):

    git clone https://github.com/zephyrproject-rtos/net-tools
    cd net-tools
    sudo ./net-setup.sh start

Terminal #2 (from workspace root, build and run this sample on ``native_sim``):

    cd ${workspaceFolder}
    west build -t run -b native_sim -p always bacnet/zephyr/samples/profiles/b-ss/

Terminal #1 (cleanup when done):

    cd net-tools
    sudo ./net-setup.sh stop

Using the Shell
***************

For ``native_sim``, shell is exposed on a pseudo-terminal. After starting the
sample, look for output similar to ``UART connected to pseudotty: /dev/pts/5``
and attach to that device:

    screen /dev/pts/<N>

On hardware boards, shell is available via virtual communication port:

    picocom --baud 115200 /dev/ttyACM0

    Terminal ready
    *** Booting Zephyr OS build v3.7.0 ***
    [00:00:00.012,000] <inf> bacnet: BACnet Device: BACnet Smart Sensor (B-SS)
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Version 1.4.1
    [00:00:00.012,000] <inf> bacnet: BACnet Stack Max APDU: 1476
    uart:~$
    bacnet   clear    device   devmem   help     history  kernel   net
    rem      resize   retval   shell    stats
    uart:~$ bacnet objects
    {"object-list": [
    {"object-identifier":{"device":4194303}},
    {"object-identifier":{"analog-input":1}},
    {"object-identifier":{"network-port":0}}],
    "object-list-size": 3}
