<!--
SPDX-FileCopyrightText: Copyright The BACnet Stack Zephyr Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Contributing to the BACnet Stack Zephyr Project

Thank you for your interest in contributing to this BACnet Stack + Zephyr
workspace. This repository combines the vendored BACnet Stack library with a
Zephyr-based build and sample environment for embedded BACnet devices.

The project aims to preserve BACnet protocol behavior while adapting the
library to Zephyr conventions, sample applications, Kconfig, and test flows.

## Repository scope

This repository has a few distinct layers, and changes should stay in the
appropriate layer:

- `bacnet/stack/`: the vendored BACnet Stack protocol library. Preserve the
  library's existing APIs and behavior unless a change specifically requires an
  API update.
- `bacnet/zephyr/`: Zephyr integration code, including Kconfig, CMake files,
  samples, subsystems, and tests.
- `zephyr/`: the local Zephyr tree used as the workspace framework.
- `west.yml`: the workspace manifest that pins the Zephyr version and the
  BACnet stack module.

When in doubt, keep BACnet semantics stable and keep Zephyr integration code in
`bacnet/zephyr/`.

## Workflow and versioning

This repository is built as a Zephyr west workspace, so development should start
from the workspace root rather than from a single directory in isolation.

Use the root workspace for validation commands such as:

```bash
west build -p always -b nucleo_f429zi bacnet/zephyr/samples/profiles/b-sa
west build -p always -b nucleo_f429zi bacnet/zephyr/samples/profiles/b-ss
west build -p always -b nucleo_f429zi bacnet/zephyr/samples/profiles/b-ld
west build -p always -b nucleo_f429zi bacnet/zephyr/samples/profiles/b-ls
west build -p always -b nucleo_f429zi bacnet/zephyr/samples/profiles/b-asc
```

If a specific board target is unavailable, prefer the nearest available sample
or a native host target such as `native_sim` for build validation.

The vendored BACnet stack library is versioned independently of the Zephyr
workspace. For library-level release work, follow the versioning and release
practices of `bacnet/stack` itself. For Zephyr integration changes, keep the
scope focused on the workspace, sample, Kconfig, or tests that are affected.

## Pre-commit hooks

This project uses `pre-commit` to check formatting and basic repository quality
before code is committed.

Set up a local Python virtual environment and install the hooks as follows:

```bash
python -m venv .venv
source .venv/bin/activate
pip install pre-commit
pre-commit install
pre-commit --version
```

This helps keep formatting, linting, and local checks consistent before review
and reduces churn in pull requests.

## Build and validation expectations

All code changes should be validated with the smallest relevant check that
exercises the modified behavior.

Useful validation commands include:

```bash
west twister -T bacnet/zephyr/tests --build-only
west twister -p native_sim -T bacnet/zephyr/tests
west twister -s bacnet/zephyr/tests/<scenario-name> --build-only
```

For sample changes, build the sample that exercises the behavior you changed.
For library changes in `bacnet/stack`, prefer the closest Zephyr sample or
Twister test that covers the same functionality.

When making a change, prefer the smallest end-to-end validation that checks the
root cause rather than running a broad suite unnecessarily.

## Coding conventions

The Zephyr integration should follow Zephyr conventions, while the BACnet stack
library should keep its own semantics and naming practices intact.

The following guidelines are expected for contributions:

- Keep the BACnet protocol semantics intact. Do not rewrite object behavior or
  service logic unless the bug requires it.
- Keep Zephyr-specific code under `bacnet/zephyr/` using Zephyr idioms: Kconfig,
  CMake, device tree overlays, logging, threads, and APIs consistent with the
  surrounding subsystem.
- Treat `bacnet/stack/` as a library dependency. Preserve compatibility and
  stable interface behavior unless an API change is explicitly required.
- Prefer small, local edits over broad refactors or unrelated cleanup.
- Match the surrounding code style: `snake_case`, explicit error handling,
  fixed-width types where appropriate, and readability over cleverness.
- New files should include the standard SPDX header.
- Keep compatibility guards minimal and clearly documented when supporting
  multiple Zephyr versions or board variants.
- Avoid board-specific hacks and non-portable assumptions unless the sample or
  test is explicitly designed for that hardware.

## Naming and API guidance

For new code in this workspace, follow the surrounding naming conventions of the
module you are editing. In practice:

- Zephyr integration code should use Zephyr-style function and file naming.
- The BACnet stack library should retain its established BACnet naming patterns
  where appropriate.
- Preserve the BACnet object model and service semantics rather than reshaping
  them for style alone.

When touching a BACnet object or service, keep the behavior consistent with the
existing stack semantics rather than introducing a reimplementation that only
looks cleaner.

## Good contribution habits

- Build the code you changed, not just the entire workspace.
- Prefer the smallest relevant sample or test to validate the fix.
- Keep the patch limited to the root cause of the issue.
- Do not claim hardware validation unless you actually ran it.
- If the fix is in the BACnet library, verify the Zephyr integration still
  builds and behaves correctly.
- If the fix is in sample code, keep it portable and readable for the supported
  Zephyr boards.

## Before submitting a patch

Before opening or merging a contribution, review the diff to confirm that it is
narrow, targeted, and directly relevant to the work.

Please also ensure that:

- the relevant sample or test has been built or run;
- the patch does not include unrelated cleanup or broad refactoring;
- the change remains compatible with the Zephyr-based embedded workflow; and
- commit messages stay scoped to the BACnet/Zephyr functionality being changed.

Thank you for helping keep the BACnet Stack + Zephyr workspace maintainable,
portable, and faithful to the BACnet protocol.
