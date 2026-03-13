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
* Removed zephyr copy of the BACnet basic objects and server framework
  and placed them into BACnet Protocol Stack library.

### Fixed
* Fixed pipeline unit test and samples build by adding shed_level and
  timer_value source files to various CMakeLists. Also fixed the
  workspace path in the Docker container for building and save
  the build logs as artifacts for debugging failures. (#61)
* Fixed bacnet shell property value JSON for empty arrays and lists. (#55)
* Fixed bacnet shell property value JSON for arrays and lists. (#54)
* Fixed missing config defines. Added Loop object and MAX_APDU
  to BACnet Kconfig options and CMakeLists.txt defines. (#53)
* Fixed compile in BACnet/IP subsys using CONFIG_POSIX_API=y. (#47)
* Fixed missing const in bip6-init.c and used zsock API.
  Replaced inet_ntoa with net_addr_ntop. Removed unnecessary select NET_SOCKETS_POSIX_NAMES. (#46)
* Fixed the BACnet light override clear shell command. (#44)
* Fixed our interface to Zephyr net manager API changes in v4.2.0. (#40)
* Fixed samples: subsys: nvs: stm32 overlays fixup.
  Don't try to delete storage partitions that doesn't exist anymore. (#39)
* Fixed warning from using FD_ISSET instead of ZSOCK_FD_ISSET. (#37)
* Fixed readme for sample projects. Fixed JSON for bacnet objects
  sub command in shell. (#31)
* Fixed network port object all datalink builds. (#26)
* Fixed basic device object to support fixes from upstream for
  BACnetARRAY and DCC and RD names.
* Fixed BACnet Basic server application thread by increasing stack size
  to avoid stack overflow.

### Changed
* Change the settings restore to load only BACnet settings by using
  bacnet_storage_handler.name feature.
* Changed the number of sample board builds for twister to reduce minutes.
* Changed pipeline to use container ghcr.io/zephyrproject-rtos/ci:v0.26-branch
  and build tests and samples using twister for Zephyr OS version 3.7 LTS
  which is built on ubuntu 22.04 image.
* Changed clang-format and pre-commit to use the style of the
  BACnet Stack library.
* Changed .gitignore with settings from Zephyr OS project.
* Changed gitignore to ignore the build folder.

### Added
* Added "bacnet property" shell command with list, size, and value options.
  The value option can list all the property values of any internal object,
  and read or write any specific property values that are writable.
  The output is in JSON format.(#52)
* Added all the basic object property value data types supported
  for B-ASC sample. (#52)
* Added BACnet Application Specific Control (B-ASC) sample. (#51)
* Added timer and audit-log objects to basic server. (#51)
* Added Model_Name config to the Kconfig for basic BACnet device. (#50)
* Added KERNEL_VERSION_MAJOR check in samples DTS overlay files to handle
  changes in Zephyr OS version 4. (#48)
* Added shell command for lighting output overridden and tracking value. (#41)
* Added BACnet zephyr shell for device date, time, and lighting
  output object. (#36)
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
