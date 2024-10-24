# bacnet-stack-zephyr

BACnet open source protocol for embedded systems
using [Zephyr OS](https://zephyrproject.org/) integrated with BACnet Stack
C library hosted on Sourceforge and Github sites.

* [Github](https://github.com/bacnet-stack/bacnet-stack)
* [Sourceforge](https://bacnet.sourceforge.net/)

# Continuous Integration

This integration uses automated continuous integration services to assist in automated compilation, validation, linting, security scanning, and unit testing to produce robust C code.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/CMake/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions/workflows/zephyr.yml) GitHub Workflow: BACnet Stack Zephyr Twister Unit Tests

# What the code does

The Zephyr OS integration offers a collection of samples in the zephyr/samples
folder that highlight the features of this BACnet integration, include some
BACnet Basic objects and services that can be used to quickly create a BACnet
device on a variety of existing boards.

These samples are crafted to be simple and easy to understand, serving as a starting point for your own projects.

## Hello BACnet Stack

A simple "Hello World" sample that can be used with any supported board boards and prints ["Hello BACnet-Stack"](./zephyr/samples/hello_bacnet_stack/README.rst) to the console.

## BACnet Application Profile Simple Sensor (B-SS)

A device application demonstrating configuration of a
[BACnet B-SS (simple sensor) device profile](./zephyr/samples/profiles/b-ss/README.rst) that can be used with any supported board boards.

# Coding Style and Guidelines

See Zephyr Project [Coding Guidelines](https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html)
