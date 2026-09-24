<!--
SPDX-FileCopyrightText: Copyright The BACnet Stack Zephyr Contributors
SPDX-License-Identifier: Apache-2.0
-->

# BACnet Stack + Zephyr RTOS agent guide

This repository integrates the BACnet Stack C library with the Zephyr
RTOS. It is not the upstream Zephyr tree; it is a Zephyr workspace and
module that provides BACnet protocol samples, subsystems, and build
support around the `bacnet/stack` library.

## Repository layout

- `bacnet/stack/`: vendored BACnet Stack library and protocol
  implementation.
- `bacnet/zephyr/`: Zephyr integration code, including Kconfig, CMake,
  samples, subsystem glue, and tests.
- `bacnet/zephyr/samples/`: sample applications, including
  profile-based BACnet devices.
- `bacnet/zephyr/tests/`: Twister-based tests for Zephyr integration.
- `west.yml`: top-level west manifest for this workspace. It pins
  Zephyr to `v3.7.1` and brings in the BACnet stack project.

The important distinction is that this repo is about BACnet on
Zephyr, not generic kernel development. When editing code, follow the
patterns of the surrounding Zephyr module and keep BACnet semantics
stable.

For the canonical contributor workflow, setup steps, validation
expectations, and submission guidance, see [../CONTRIBUTING.md](../CONTRIBUTING.md).
This document focuses on repository-specific agent behavior, while the
contribution guide is the project-wide reference for contributors.

## Build flow

Use a west workspace and build from the workspace root, not from the
`bacnet/` directory by itself.

Common build examples:

- `west build -p always -b nucleo_f429zi`
  `bacnet/zephyr/samples/profiles/b-sa`
- `west build -p always -b nucleo_f429zi`
  `bacnet/zephyr/samples/profiles/b-ss`
- `west build -p always -b nucleo_f429zi`
  `bacnet/zephyr/samples/profiles/b-ld`
- `west build -p always -b nucleo_f429zi`
  `bacnet/zephyr/samples/profiles/b-ls`
- `west build -p always -b nucleo_f429zi`
  `bacnet/zephyr/samples/profiles/b-asc`
- `west build -p always -b native_sim`
  `bacnet/zephyr/samples/hello_bacnet_stack`

Useful runtime commands:

- `west flash -d build`
- `west debug -d build`

If a board target is not available, prefer a build-only validation on
the affected sample or a `native_sim` target.

For the project-wide workflow and contributor expectations, also refer
to [../CONTRIBUTING.md](../CONTRIBUTING.md) before finalizing a
change.

## Validation

For code changes, validate the smallest relevant end-to-end check:

- `west twister -T bacnet/zephyr/tests --build-only`
- `west twister -p native_sim -T bacnet/zephyr/tests`
- `west twister -s bacnet/zephyr/tests/<scenario-name> --build-only`

When you change a BACnet sample, build the specific sample that
exercises the affected behavior. When you change the BACnet stack
library, prefer the closest Zephyr sample or Twister test that covers
the same feature.

## Project-specific coding rules

- Keep BACnet protocol semantics intact. Do not rewrite BACnet object
  behavior or service logic just to make it "cleaner" unless the bug
  requires it.
- Keep Zephyr-specific integration code in `bacnet/zephyr/` using
  Zephyr conventions: Kconfig, CMake, device tree overlays, logging,
  threads, and APIs that match the surrounding subsystem.
- Treat `bacnet/stack/` as a protocol library dependency. Preserve
  compatibility and existing interfaces unless the change explicitly
  requires an API change.
- Prefer small, local edits over broad refactors. Avoid unrelated
  cleanup in the same patch.
- Match the surrounding file style: `snake_case`, Zephyr idioms,
  explicit error checking, and fixed-width integer types where
  appropriate.
- New files should include the usual SPDX header information.
- Keep compatibility guards minimal and documented when support for
  multiple Zephyr releases is required.
- Do not add board-specific hacks or non-portable assumptions unless
  the sample or test is explicitly designed for that board.

## Good agent behavior

- Build what you changed, not just the repo overall.
- Prefer the smallest affected sample/test over broad suite runs.
- Keep changes limited to the root cause.
- Do not claim hardware validation unless it was actually run.
- If a fix is in the BACnet library, verify the Zephyr side still
  builds and behaves correctly.
- If a fix is in sample code, ensure it remains portable and readable
  for the supported Zephyr boards.

## Before submitting

- Review the diff and confirm it is narrow and directly related to the
  task.
- Run the relevant sample or Twister validation before claiming the fix
  is ready.
- Follow the project contribution workflow in [../CONTRIBUTING.md](../CONTRIBUTING.md),
  including setup, pre-commit, and validation expectations.
