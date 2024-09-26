# Copyright (c) 2024 Legrand North America, LLC.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_BACDL_BIP
  bip-init.c
)

zephyr_sources_ifdef(
  CONFIG_BACDL_BIP6
  bip6-init.c
)
