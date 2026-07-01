.. _b-asc_sample:

BACnet Profile B-ASC Sample
###########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet Application Specific Controller (B-ASC) device profile.

L.4.3 BACnet Application Specific Controller (B-ASC)
****************************************************

A B-ASC is a controller with limited resources relative to an
Advanced Application Controller (B-AAC).
A B-ASC is intended for use in a specific application
and supports limited programmability.

Data Sharing
************

* Ability to provide the values of any of its BACnet objects
* Ability to allow modification of some or all of its BACnet
  objects by another device

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

This sample can be found under :bacnet_file:`samples/profiles/b-asc` in
the BACnet tree.

The sample can be built for several platforms - use `west boards` to
list the supported boards.

Compile this sample for the `nucleo_f429zi` board:

    west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-asc/
    west flash

Compile this sample for the `rpi_pico` board:

    west build -b rpi_pico -p always bacnet/zephyr/samples/profiles/b-asc/
    west flash

Compile this sample for the `adafruit_grand_central_m4_express` board with

    west build -b adafruit_grand_central_m4_express -p always bacnet/zephyr/samples/profiles/b-asc/
    west flash

Compile and run this sample for the `native_sim` board:

    west build -t run -b native_sim -p always bacnet/zephyr/samples/profiles/b-asc/

End-to-end ``native_sim`` networking setup flow (Linux host):

Terminal #1 (set up host TAP networking using Zephyr net-tools):

    git clone https://github.com/zephyrproject-rtos/net-tools
    cd net-tools
    sudo ./net-setup.sh start

Terminal #2 (from workspace root, build and run this sample on ``native_sim``):

    cd ${workspaceFolder}
    west build -t run -b native_sim -p always bacnet/zephyr/samples/profiles/b-asc/

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
    *** Booting Zephyr OS build v3.7.1 ***
    uart:~$
    bacnet   clear    device   devmem   help     history  kernel   net
    rem      resize   retval   shell    stats
    uart:~$ bacnet object list
    {"object-list": [
    {"object-identifier":{"device":260127}},
    {"object-identifier":{"analog-input":1}},
    {"object-identifier":{"analog-output":1}},
    {"object-identifier":{"analog-value":1}},
    {"object-identifier":{"audit-log":1}},
    {"object-identifier":{"binary-input":1}},
    {"object-identifier":{"binary-output":1}},
    {"object-identifier":{"binary-value":1}},
    {"object-identifier":{"multi-state-input":1}},
    {"object-identifier":{"multi-state-output":1}},
    {"object-identifier":{"multi-state-value":1}},
    {"object-identifier":{"network-port":1}},
    {"object-identifier":{"calendar":1}},
    {"object-identifier":{"integer-value":1}},
    {"object-identifier":{"life-safety-point":1}},
    {"object-identifier":{"load-control":1}},
    {"object-identifier":{"lighting-output":1}},
    {"object-identifier":{"channel":1}},
    {"object-identifier":{"binary-lighting-output":1}},
    {"object-identifier":{"color":1}},
    {"object-identifier":{"color-temperature":1}},
    {"object-identifier":{"file":1}},
    {"object-identifier":{"structured-view":1}},
    {"object-identifier":{"characterstring-value":1}},
    {"object-identifier":{"time-value":1}},
    {"object-identifier":{"timer":1}}],
    "object-list-size": 26}
    uart:~$
