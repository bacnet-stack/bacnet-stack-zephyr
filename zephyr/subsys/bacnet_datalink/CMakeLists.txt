# Copyright (c) 2024 Legrand North America, LLC.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  $<$<BOOL:${CONFIG_BACDL_BIP}>:bip-init.c>
  $<$<BOOL:${CONFIG_BACDL_BIP6}>:bip6-init.c>
)
