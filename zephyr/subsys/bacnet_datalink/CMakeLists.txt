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

zephyr_sources_ifdef(
  CONFIG_BACDL_BSC
  bsc-event.c
  websocket-global.c
  websocket-cli.c
  websocket-srv.c
  libc_extensions.c
)
