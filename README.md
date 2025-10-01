# bacnet-stack-zephyr

BACnet open source protocol for embedded systems
using [Zephyr OS](https://zephyrproject.org/) integrated with BACnet Stack
C library hosted on Sourceforge and Github sites.

* [Github](https://github.com/bacnet-stack/bacnet-stack)
* [Sourceforge](https://bacnet.sourceforge.net/)

# Continuous Integration

This integration uses automated continuous integration services to assist
in automated compilation, validation, linting, security scanning, and unit
testing to produce robust C code.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack-zephyr/actions/workflows/zephyr.yml/badge.svg)](https://github.com/bacnet-stack/bacnet-stack-zephyr/actions/workflows/zephyr.yml) GitHub Workflow: BACnet Stack Zephyr Twister Unit Tests

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack-zephyr/actions/workflows/zephyr-samples.yml/badge.svg)](https://github.com/bacnet-stack/bacnet-stack-zephyr/actions/workflows/zephyr-samples.yml) GitHub Workflow: BACnet Stack Samples Built

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack-zephyr/workflows/CodeQL/badge.svg)](https://github.com/bacnet-stack-zephyr/bacnet-stack/actions/workflows/codeql-analysis.yml) GitHub Workflow: CodeQL Analysis

# What the code does

The Zephyr OS integration offers a collection of samples in the zephyr/samples
folder that highlight the features of this BACnet integration, including some
BACnet Basic dynamically created objects and services that can be used to
quickly create a custom BACnet device on a variety of existing boards.

These samples are crafted to be simple and easy to understand, serving as a
starting point for your own projects.

This repository is a Zephyr manifest-module, which means it can be used in either
of the following ways:

- As a module brought in by a different manifest (recommended for product development)

  **[Prototyped but not verified]** Add the following into the root manifest file (e.g., $workspace_dir/$manifest_repo/west.yml)

  ```
  # In external manifest repository, `west.yml`
  # to bring in the Zephyr integration of bacnet-stack
  # as a Zephyr module

    # Add the following under remotes:
      - name: bacnet-stack
        url-base: https://github.com/bacnet-stack

    # Add the following under projects:
      - name: bacnet
        repo-path: bacnet-stack-zephyr
        path: modules/lib/bacnet
        import:
          name-allowlist:
            - bacnet-stack

      - name: bacnet-stack
        path: modules/lib/bacnet/stack
  ```

  - As a workspace manifest repository (recommended for bacnet-stack contributions):

    `west init -m https://github.com/bacnet-stack/bacnet-stack-zephyr --mr default $my_workspace`

  - Note that as Zephyr OS versions change, their API often changes.
    This library will use the following methods to accommodate the changes:
    1. Use defines from <zephyr/version.h> for API changes:
       ```
       #if ZEPHYR_VERSION_CODE >= ZEPHYR_VERSION(4,2,0)
       typedef uint64_t mgmt_event_t;
       #else
       typedef uint32_t mgmt_event_t;
       #endif
       ```
    2. In CMakeLists.txt via trying to load specific versions of the Zephyr
       package or checking using:
       ```
       if ("${KERNEL_VERSION_STRING}" VERSION_GREATER_EQUAL "4.2.0")
       # Do stuff...
       endif()
       ```
    3. In Kconfig by using 2 above and trying to load specific versions
       of the Zephyr plugin and updating CONF_FILE accordingly.

## Hello BACnet Stack

A simple "Hello World" sample that can be used with any supported board boards
and prints ["Hello BACnet-Stack"](./zephyr/samples/hello_bacnet_stack/README.rst)
to the console.

## BACnet Standardized Device Profiles

### Device Profile - BACnet Smart Actuator (B-SA)

A device application demonstrating configuration of a
[BACnet Smart Actuator (B-SA) device profile](./zephyr/samples/profiles/b-sa/README.rst)
that can be used with any supported boards.

### Device Profile - BACnet Smart Sensor (B-SS)

A device application demonstrating configuration of a
[BACnet Smart Sensor (B-SS) device profile](./zephyr/samples/profiles/b-ss/README.rst)
that can be used with any supported boards.

### Device Profile - BACnet Lighting Device (B-LD)

A device application demonstrating configuration of a
[BACnet Lighting Device (B-LD) device profile](./zephyr/samples/profiles/b-ld/README.rst)
that can be used with any supported boards.

### Device Profile - BACnet Lighting Supervisor (B-LS)

A device application demonstrating configuration of a
[BACnet Lighting Supervisor (B-LS) device profile](./zephyr/samples/profiles/b-ls/README.rst)
that can be used with any supported boards.

# Coding Style and Guidelines

See Zephyr Project [Coding Guidelines](https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html)
