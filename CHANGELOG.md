# BACnet Stack Zephry OS ChangeLog

BACnet open source protocol stack C library module used with Zephyr OS

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The git repository is hosted at the following site:
* https://github.com/bacnet-stack/bacnet-stack-zephyr

## [Unreleased]

### Security

### Removed
* Removed BACnet basic objects and server framework into BACnet libraray
  and removed zephyr version.

### Fixed
* Fixed readme for sample projects. Fixed JSON for bacnet objects
  sub command in shell. (#31)
* Fixed network port object all datalink builds. (#26)
* Fixed basic device object to support fixes from upstream for
  BACnetARRAY and DCC and RD names.
* Fixed BACnet Basic server application thread by increasing stack size
  to avoid stack overflow.

### Changed
* Changed the number of sample board builds for twister to reduce minutes.
* Changed pipeline to use container ghcr.io/zephyrproject-rtos/ci:v0.26-branch
  and build tests and samples using twister for Zephyr OS version 3.7 LTS
  which is built on ubuntu 22.04 image.
* Changed clang-format and pre-commit to use the style of the
  BACnet Stack library.
* Changed .gitignore with settings from Zephyr OS project.
* Changed gitignore to ignore the build folder.

### Added
* Added KERNEL_VERSION_MAJOR check in samples DTS overlay files to handle
  changes in Zephyr OS version 4
* Added BACDL ZIGBEE and BSC datalink defines to Kconfig & CMakeLists.txt (#35)
* Added baclog, you-are, who-am-i, create-object, delete-object, write-group,
  bramfs, bsramfs, and color-rgb modules to cmake. (#35)
* Added zephyr settings_handler to restore data using settings_load()
  after initialization. Changed shell for bacnet settings and storage
  get and set values to use BACnet Stack parse and snprintf.
  Added BACnet settings subsys to zephyr sample profiles
  B-LD B-LS B-SA and B-SS. (#33)
* Added BACnet Settings subys into the profile samples (#30)
* Added espressif and rpi_pico HAL to west for more sample testing.
* Added Load Control object to zephyr basic device
* Added BACnet Storage configuration NVS for storing BACnet settings.
  Added overlay for nucleo_f429zi board to place storage map at the
  correct place in memory. (#22)
* Added BACDL_CUSTOM and BACDL_MULTIPLE to Kconfig for datalink options.
* Added BACnet Lighting Device B-LD profile sample
* Added BACnet Lighting Supervisor B-LS profile sample. (#21)

## [1.4.0] - 2024-11-11
* Initial release of BACnet Stack v1.4 on Zephyr v3.7.0
