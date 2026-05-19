# Copyright (c) 2020 Legrand North America, LLC.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Detect the platform reliably
if(ZEPHYR_BASE)
  if (NOT CONFIG_BACNETSTACK)
    return()
  endif()
  set(ZEPHYR YES)
else()
  message(FATAL_ERROR "ZEPHYR_BASE needs to be defined for Zephyr builds")
endif()

#
# options managed through Kconfig and use names CONFIG_*
#
message(STATUS "BACNETSTACK: using cmake ${CMAKE_VERSION}")
message(STATUS "BACNETSTACK: CMAKE_C_COMPILER_ID \"${CMAKE_C_COMPILER_ID}\"")
message(STATUS "BACNETSTACK: CMAKE_C_COMPILER_VERSION \"${CMAKE_C_COMPILER_VERSION}\"")
message(STATUS "BACNETSTACK: CMAKE_CXX_COMPILER_ID \"${CMAKE_CXX_COMPILER_ID}\"")
message(STATUS "BACNETSTACK: CMAKE_CXX_COMPILER_VERSION \"${CMAKE_CXX_COMPILER_VERSION}\"")
message(STATUS "BACNETSTACK: CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\"")
message(STATUS "BACNETSTACK: BACNET_VENDOR_NAME \"${CONFIG_BACNET_VENDOR_NAME}\"")
message(STATUS "BACNETSTACK: BACNET_VENDOR_IDENTIFIER \"${CONFIG_BACNET_VENDOR_IDENTIFIER}\"")
message(STATUS "BACNETSTACK: BACNET_PROTOCOL_REVISION \"${CONFIG_BACNET_PROTOCOL_REVISION}\"")
message(STATUS "BACNETSTACK: BACAPP_MINIMAL \"${CONFIG_BACAPP_MINIMAL}\"")
message(STATUS "BACNETSTACK: BACAPP_TYPES_EXTRA \"${CONFIG_BACAPP_TYPES_EXTRA}\"")
message(STATUS "BACNETSTACK: BACNET_ADDRESS_CACHE_FILE \"${CONFIG_BACNET_ADDRESS_CACHE_FILE}\"")
message(STATUS "BACNETSTACK: MAX_APDU \"${CONFIG_BACNET_MAX_APDU_SIZE}\"")
message(STATUS "BACNETSTACK: MAX_TSM_TRANSACTIONS \"${CONFIG_BACNET_MAX_TSM_TRANSACTIONS}\"")
message(STATUS "BACNETSTACK: MAX_ADDRESS_CACHE \"${CONFIG_BACNET_MAX_ADDRESS_CACHE}\"")
message(STATUS "BACNETSTACK: MAX_CHARACTER_STRING_BYTES \"${CONFIG_BACNET_MAX_CHARACTER_STRING_BYTES}\"")
message(STATUS "BACNETSTACK: MAX_OCTET_STRING_BYTES \"${CONFIG_BACNET_MAX_OCTET_STRING_BYTES}\"")
message(STATUS "BACNETSTACK: BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE \"${CONFIG_BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE}\"")
message(STATUS "BACNETSTACK: CONFIG_BACNETSTACK_LOG_LEVEL \"${CONFIG_BACNETSTACK_LOG_LEVEL}\"")
message(STATUS "BACNETSTACK: BACNET_BACKUP_RESTORE \"${CONFIG_BACNET_BASIC_BACKUP_RESTORE}\"")

if(NOT (CONFIG_BACDL_ETHERNET OR
  CONFIG_BACDL_MSTP OR
  CONFIG_BACDL_ARCNET OR
  CONFIG_BACDL_BIP OR
  CONFIG_BACDL_BIP6 OR
  CONFIG_BACDL_ZIGBEE OR
  CONFIG_BACDL_BSC OR
  CONFIG_BACDL_CUSTOM OR
  CONFIG_BACDL_MULTIPLE OR
  CONFIG_BACDL_NONE OR
  CONFIG_BACAPP_ALL))
  set(CONFIG_BACDL_NONE YES)
endif()

message(STATUS "BACNETSTACK: BACDL_BIP6 \"${CONFIG_BACDL_BIP6}\"")
message(STATUS "BACNETSTACK: BACDL_BIP \"${CONFIG_BACDL_BIP}\"")
message(STATUS "BACNETSTACK: BACDL_ARCNET \"${CONFIG_BACDL_ARCNET}\"")
message(STATUS "BACNETSTACK: BACDL_MSTP \"${CONFIG_BACDL_MSTP}\"")
message(STATUS "BACNETSTACK: BACDL_ETHERNET \"${CONFIG_BACDL_ETHERNET}\"")
message(STATUS "BACNETSTACK: BACDL_ZIGBEE \"${CONFIG_BACDL_ZIGBEE}\"")
message(STATUS "BACNETSTACK: BACDL_BSC \"${CONFIG_BACDL_BSC}\"")
message(STATUS "BACNETSTACK: BACDL_CUSTOM \"${CONFIG_BACDL_CUSTOM}\"")
message(STATUS "BACNETSTACK: BACDL_MULTIPLE \"${CONFIG_BACDL_MULTIPLE}\"")
message(STATUS "BACNETSTACK: BACDL_NONE \"${CONFIG_BACDL_NONE}\"")
message(STATUS "BACNETSTACK: BACAPP_ALL \"${CONFIG_BACAPP_ALL}\"")

#Do not allow in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#
# sources
#
set(BACNETSTACK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../stack)
set(BACNETSTACK_SRC ${BACNETSTACK_ROOT}/src)
set(BACNETSTACK_POSIX ${BACNETSTACK_ROOT}/ports/posix)

set(BACNETSTACK_SRCS
    ${BACNETSTACK_SRC}/bacnet/abort.c
    ${BACNETSTACK_SRC}/bacnet/abort.h
    ${BACNETSTACK_SRC}/bacnet/access_rule.c
    ${BACNETSTACK_SRC}/bacnet/access_rule.h
    ${BACNETSTACK_SRC}/bacnet/alarm_ack.c
    ${BACNETSTACK_SRC}/bacnet/alarm_ack.h
    ${BACNETSTACK_SRC}/bacnet/apdu.h
    ${BACNETSTACK_SRC}/bacnet/arf.c
    ${BACNETSTACK_SRC}/bacnet/arf.h
    ${BACNETSTACK_SRC}/bacnet/assigned_access_rights.c
    ${BACNETSTACK_SRC}/bacnet/assigned_access_rights.h
    ${BACNETSTACK_SRC}/bacnet/authentication_factor.c
    ${BACNETSTACK_SRC}/bacnet/authentication_factor_format.c
    ${BACNETSTACK_SRC}/bacnet/authentication_factor_format.h
    ${BACNETSTACK_SRC}/bacnet/authentication_factor.h
    ${BACNETSTACK_SRC}/bacnet/awf.c
    ${BACNETSTACK_SRC}/bacnet/awf.h
    ${BACNETSTACK_SRC}/bacnet/bacaction.c
    ${BACNETSTACK_SRC}/bacnet/bacaction.h
    ${BACNETSTACK_SRC}/bacnet/bacaddr.c
    ${BACNETSTACK_SRC}/bacnet/bacaddr.h
    ${BACNETSTACK_SRC}/bacnet/bacapp.c
    ${BACNETSTACK_SRC}/bacnet/bacapp.h
    ${BACNETSTACK_SRC}/bacnet/bacaudit.c
    ${BACNETSTACK_SRC}/bacnet/bacaudit.h
    ${BACNETSTACK_SRC}/bacnet/bacdcode.c
    ${BACNETSTACK_SRC}/bacnet/bacdcode.h
    ${BACNETSTACK_SRC}/bacnet/bacdef.h
    ${BACNETSTACK_SRC}/bacnet/bacdest.c
    ${BACNETSTACK_SRC}/bacnet/bacdest.h
    ${BACNETSTACK_SRC}/bacnet/bacdevobjpropref.c
    ${BACNETSTACK_SRC}/bacnet/bacdevobjpropref.h
    ${BACNETSTACK_SRC}/bacnet/bacenum.h
    ${BACNETSTACK_SRC}/bacnet/bacerror.c
    ${BACNETSTACK_SRC}/bacnet/bacerror.h
    ${BACNETSTACK_SRC}/bacnet/bacint.c
    ${BACNETSTACK_SRC}/bacnet/bacint.h
    ${BACNETSTACK_SRC}/bacnet/baclog.c
    ${BACNETSTACK_SRC}/bacnet/baclog.h
    ${BACNETSTACK_SRC}/bacnet/bacprop.c
    ${BACNETSTACK_SRC}/bacnet/bacprop.h
    ${BACNETSTACK_SRC}/bacnet/bacpropstates.c
    ${BACNETSTACK_SRC}/bacnet/bacpropstates.h
    ${BACNETSTACK_SRC}/bacnet/bacreal.c
    ${BACNETSTACK_SRC}/bacnet/bacreal.h
    ${BACNETSTACK_SRC}/bacnet/bacstr.c
    ${BACNETSTACK_SRC}/bacnet/bacstr.h
    ${BACNETSTACK_SRC}/bacnet/bactext.c
    ${BACNETSTACK_SRC}/bacnet/bactext.h
    ${BACNETSTACK_SRC}/bacnet/bactimevalue.c
    ${BACNETSTACK_SRC}/bacnet/bactimevalue.h
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd/h_bbmd.c>
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd/h_bbmd.h>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.h>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bvlc-sc.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bvlc-sc.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-socket.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-socket.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-util.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-util.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-hub-connector.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-hub-connector.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-hub-function.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-hub-function.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-node-switch.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-node-switch.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-node.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-node.c>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-datalink.h>
    $<$<BOOL:${CONFIG_BACDL_BSC}>:${BACNETSTACK_SRC}/bacnet/datalink/bsc/bsc-datalink.c>
    ${BACNETSTACK_SRC}/bacnet/basic/binding/address.c
    ${BACNETSTACK_SRC}/bacnet/basic/binding/address.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_npdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_npdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_routed_npdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_routed_npdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/s_router.c
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/s_router.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_credential.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_door.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_point.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_rights.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_user.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_zone.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/acc.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ai.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ao.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/av.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bacfile.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bi.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bitstring_value.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/blo.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bo.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/calendar.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/channel.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/color_object.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/color_temperature.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/command.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/credential_data_input.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/csv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/device.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/iv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lc.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lo.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lsp.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lsz.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ms-input.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/mso.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/msv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/nc.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/netport.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/objects.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/osv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/piv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/program.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/schedule.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/structured_view.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/time_value.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/timer.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/trendlog.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_alarm_ack.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_apdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_apdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_awf.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ccov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ccov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_cov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_cov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_create_object.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_delete_object.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_dcc.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_dcc.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_gas_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_iam.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_iam.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ihave.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ihave.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_lso.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_noserv.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_noserv.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rd.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rd.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ts.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ucov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_upt.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whoami.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whoami.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whohas.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whohas.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whois.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whois.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_write_group.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_write_group.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_youare.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_youare.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_abort.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ack_alarm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_arfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_awfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_create_object.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_create_object.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_dcc.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_delete_object.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_delete_object.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_error.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_event.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_getevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_iam.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_iam.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ihave.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ihave.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_list_element.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_list_element.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_lso.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rd.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_readrange.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ts.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_uevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_upt.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whoami.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whohas.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whohas.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whois.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whois.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_write_group.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_write_group.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_youare.h
    ${BACNETSTACK_SRC}/bacnet/basic/services.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bigend.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bigend.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/datetime_mstimer.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bramfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bramfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bsramfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bsramfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/color_rgb.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/color_rgb.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/days.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/days.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/debug.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/debug.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/dst.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/dst.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/fifo.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/fifo.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/filename.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/filename.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/key.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/keylist.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/keylist.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/linear.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/linear.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/lighting_command.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/lighting_command.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/mstimer.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/mstimer.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/ringbuf.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/ringbuf.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/sbuf.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/sbuf.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/state_name.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/state_name.h
    ${BACNETSTACK_SRC}/bacnet/basic/tsm/tsm.c
    ${BACNETSTACK_SRC}/bacnet/basic/tsm/tsm.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bits.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bytes.h
    ${BACNETSTACK_SRC}/bacnet/calendar_entry.c
    ${BACNETSTACK_SRC}/bacnet/calendar_entry.h
    ${BACNETSTACK_SRC}/bacnet/channel_value.c
    ${BACNETSTACK_SRC}/bacnet/channel_value.h
    ${BACNETSTACK_SRC}/bacnet/config.h
    ${BACNETSTACK_SRC}/bacnet/cov.c
    ${BACNETSTACK_SRC}/bacnet/cov.h
    ${BACNETSTACK_SRC}/bacnet/create_object.c
    ${BACNETSTACK_SRC}/bacnet/create_object.h
    ${BACNETSTACK_SRC}/bacnet/credential_authentication_factor.c
    ${BACNETSTACK_SRC}/bacnet/credential_authentication_factor.h
    ${BACNETSTACK_SRC}/bacnet/dailyschedule.c
    ${BACNETSTACK_SRC}/bacnet/dailyschedule.h
    ${BACNETSTACK_SRC}/bacnet/datalink/arcnet.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bacsec.c
    ${BACNETSTACK_SRC}/bacnet/datalink/bacsec.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bip6.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bip.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bvlc6.c
    ${BACNETSTACK_SRC}/bacnet/datalink/bvlc6.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bvlc.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bvlc.c
    ${BACNETSTACK_SRC}/bacnet/datalink/cobs.h
    ${BACNETSTACK_SRC}/bacnet/datalink/cobs.c
    ${BACNETSTACK_SRC}/bacnet/datalink/crc.h
    ${BACNETSTACK_SRC}/bacnet/datalink/crc.c
    ${BACNETSTACK_SRC}/bacnet/datalink/datalink.c
    ${BACNETSTACK_SRC}/bacnet/datalink/datalink.h
    ${BACNETSTACK_SRC}/bacnet/datalink/dlmstp.c
    ${BACNETSTACK_SRC}/bacnet/datalink/dlmstp.h
    ${BACNETSTACK_SRC}/bacnet/datalink/ethernet.h
    ${BACNETSTACK_SRC}/bacnet/datalink/mstp.c
    ${BACNETSTACK_SRC}/bacnet/datalink/mstp.h
    ${BACNETSTACK_SRC}/bacnet/datalink/mstpdef.h
    ${BACNETSTACK_SRC}/bacnet/datalink/mstptext.c
    ${BACNETSTACK_SRC}/bacnet/datalink/mstptext.h
    ${BACNETSTACK_SRC}/bacnet/datetime.c
    ${BACNETSTACK_SRC}/bacnet/datetime.h
    ${BACNETSTACK_SRC}/bacnet/dcc.c
    ${BACNETSTACK_SRC}/bacnet/dcc.h
    ${BACNETSTACK_SRC}/bacnet/delete_object.c
    ${BACNETSTACK_SRC}/bacnet/delete_object.h
    ${BACNETSTACK_SRC}/bacnet/event.c
    ${BACNETSTACK_SRC}/bacnet/event.h
    ${BACNETSTACK_SRC}/bacnet/get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/getevent.c
    ${BACNETSTACK_SRC}/bacnet/getevent.h
    ${BACNETSTACK_SRC}/bacnet/hostnport.c
    ${BACNETSTACK_SRC}/bacnet/hostnport.h
    ${BACNETSTACK_SRC}/bacnet/iam.c
    ${BACNETSTACK_SRC}/bacnet/iam.h
    ${BACNETSTACK_SRC}/bacnet/ihave.c
    ${BACNETSTACK_SRC}/bacnet/ihave.h
    ${BACNETSTACK_SRC}/bacnet/indtext.c
    ${BACNETSTACK_SRC}/bacnet/indtext.h
    ${BACNETSTACK_SRC}/bacnet/lighting.c
    ${BACNETSTACK_SRC}/bacnet/lighting.h
    ${BACNETSTACK_SRC}/bacnet/list_element.c
    ${BACNETSTACK_SRC}/bacnet/list_element.h
    ${BACNETSTACK_SRC}/bacnet/lso.c
    ${BACNETSTACK_SRC}/bacnet/lso.h
    ${BACNETSTACK_SRC}/bacnet/memcopy.c
    ${BACNETSTACK_SRC}/bacnet/memcopy.h
    ${BACNETSTACK_SRC}/bacnet/npdu.c
    ${BACNETSTACK_SRC}/bacnet/npdu.h
    ${BACNETSTACK_SRC}/bacnet/property.c
    ${BACNETSTACK_SRC}/bacnet/property.h
    ${BACNETSTACK_SRC}/bacnet/proplist.c
    ${BACNETSTACK_SRC}/bacnet/proplist.h
    ${BACNETSTACK_SRC}/bacnet/ptransfer.c
    ${BACNETSTACK_SRC}/bacnet/ptransfer.h
    ${BACNETSTACK_SRC}/bacnet/rd.c
    ${BACNETSTACK_SRC}/bacnet/rd.h
    ${BACNETSTACK_SRC}/bacnet/readrange.c
    ${BACNETSTACK_SRC}/bacnet/readrange.h
    ${BACNETSTACK_SRC}/bacnet/reject.c
    ${BACNETSTACK_SRC}/bacnet/reject.h
    ${BACNETSTACK_SRC}/bacnet/rp.c
    ${BACNETSTACK_SRC}/bacnet/rp.h
    ${BACNETSTACK_SRC}/bacnet/rpm.c
    ${BACNETSTACK_SRC}/bacnet/rpm.h
    ${BACNETSTACK_SRC}/bacnet/secure_connect.c
    ${BACNETSTACK_SRC}/bacnet/secure_connect.h
    ${BACNETSTACK_SRC}/bacnet/shed_level.c
    ${BACNETSTACK_SRC}/bacnet/shed_level.h
    ${BACNETSTACK_SRC}/bacnet/special_event.c
    ${BACNETSTACK_SRC}/bacnet/special_event.h
    ${BACNETSTACK_SRC}/bacnet/timer_value.c
    ${BACNETSTACK_SRC}/bacnet/timer_value.h
    ${BACNETSTACK_SRC}/bacnet/timestamp.c
    ${BACNETSTACK_SRC}/bacnet/timestamp.h
    ${BACNETSTACK_SRC}/bacnet/timesync.c
    ${BACNETSTACK_SRC}/bacnet/timesync.h
    ${BACNETSTACK_SRC}/bacnet/version.h
    ${BACNETSTACK_SRC}/bacnet/whoami.c
    ${BACNETSTACK_SRC}/bacnet/whoami.h
    ${BACNETSTACK_SRC}/bacnet/weeklyschedule.c
    ${BACNETSTACK_SRC}/bacnet/weeklyschedule.h
    ${BACNETSTACK_SRC}/bacnet/whohas.c
    ${BACNETSTACK_SRC}/bacnet/whohas.h
    ${BACNETSTACK_SRC}/bacnet/whois.c
    ${BACNETSTACK_SRC}/bacnet/whois.h
    ${BACNETSTACK_SRC}/bacnet/wp.c
    ${BACNETSTACK_SRC}/bacnet/wp.h
    ${BACNETSTACK_SRC}/bacnet/wpm.c
    ${BACNETSTACK_SRC}/bacnet/wpm.h
    ${BACNETSTACK_SRC}/bacnet/write_group.c
    ${BACNETSTACK_SRC}/bacnet/write_group.h
    ${BACNETSTACK_SRC}/bacnet/youare.c
    ${BACNETSTACK_SRC}/bacnet/youare.h
    )

message(STATUS "BACNETSTACK: CONFIG_BACNET_BASIC_OBJECT_DEVICE_SERVER \"${CONFIG_BACNET_BASIC_OBJECT_DEVICE_SERVER}\"")
message(STATUS "BACNETSTACK: CONFIG_BACNET_BASIC_OBJECT_DEVICE_CLIENT \"${CONFIG_BACNET_BASIC_OBJECT_DEVICE_CLIENT}\"")
message(STATUS "BACNETSTACK: CONFIG_BACNET_BASIC_OBJECT_NETWORK_PORT \"${CONFIG_BACNET_BASIC_OBJECT_NETWORK_PORT}\"")
message(STATUS "BACNETSTACK: CONFIG_BACNET_BASIC_OBJECT_ANALOG_INPUT \"${CONFIG_BACNET_BASIC_OBJECT_ANALOG_INPUT}\"")

set(BACNETSTACK_BASIC_SRCS
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.c>
    $<$<BOOL:${CONFIG_BACDL_ZIGBEE}>:${BACNETSTACK_SRC}/bacnet/basic/bzll/bzllvmac.c>
    $<$<BOOL:${CONFIG_BACDL_ZIGBEE}>:${BACNETSTACK_SRC}/bacnet/basic/bzll/h_bzll.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_basic.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_device.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_port.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_port_ipv4.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_port_ipv4.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/server/bacnet_port_bzll.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_credential.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_door.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_point.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_rights.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_user.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/access_zone.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_ACCUMULATOR}>:${BACNETSTACK_SRC}/bacnet/basic/object/acc.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_ANALOG_INPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/ai.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_ANALOG_OUTPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/ao.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_ANALOG_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/av.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_AUDIT_LOG}>:${BACNETSTACK_SRC}/bacnet/basic/object/auditlog.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_FILE}>:${BACNETSTACK_SRC}/bacnet/basic/object/bacfile.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_BINARY_INPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/bi.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_BINARY_OUTPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/bo.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_BINARY_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/bv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_CALENDAR}>:${BACNETSTACK_SRC}/bacnet/basic/object/calendar.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_CHANNEL}>:${BACNETSTACK_SRC}/bacnet/basic/object/channel.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_COMMAND}>:${BACNETSTACK_SRC}/bacnet/basic/object/command.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_COLOR}>:${BACNETSTACK_SRC}/bacnet/basic/object/color_object.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_COLOR_TEMPERATURE}>:${BACNETSTACK_SRC}/bacnet/basic/object/color_temperature.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS_ACCESS}>:${BACNETSTACK_SRC}/bacnet/basic/object/credential_data_input.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_CHARACTERSTRING_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/csv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_DEVICE_CLIENT}>:${BACNETSTACK_SRC}/bacnet/basic/object/client/device-client.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_DEVICE_SERVER}>:${BACNETSTACK_SRC}/bacnet/basic/object/device.c>
    $<$<BOOL:${CONFIG_BAC_ROUTING}>:${BACNETSTACK_SRC}/bacnet/basic/object/gateway/gw_device.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_INTEGER_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/iv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_LOAD_CONTROL}>:${BACNETSTACK_SRC}/bacnet/basic/object/lc.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_LOOP}>:${BACNETSTACK_SRC}/bacnet/basic/object/loop.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_LIGHTING_OUTPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/lo.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_BINARY_LIGHTING_OUTPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/blo.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_LIFE_SAFETY_POINT}>:${BACNETSTACK_SRC}/bacnet/basic/object/lsp.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_INPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/ms-input.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_OUTPUT}>:${BACNETSTACK_SRC}/bacnet/basic/object/mso.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_MULTISTATE_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/msv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_NOTIFICATION_CLASS}>:${BACNETSTACK_SRC}/bacnet/basic/object/nc.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_NETWORK_PORT}>:${BACNETSTACK_SRC}/bacnet/basic/object/netport.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS}>:${BACNETSTACK_SRC}/bacnet/basic/object/objects.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_OCTET_STRING_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/osv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_POSITIVE_INTEGER_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/piv.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_SCHEDULE}>:${BACNETSTACK_SRC}/bacnet/basic/object/schedule.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_STRUCTURED_VIEW}>:${BACNETSTACK_SRC}/bacnet/basic/object/structured_view.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_TIME_VALUE}>:${BACNETSTACK_SRC}/bacnet/basic/object/time_value.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_TIMER}>:${BACNETSTACK_SRC}/bacnet/basic/object/timer.c>
    $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECT_TRENDLOG}>:${BACNETSTACK_SRC}/bacnet/basic/object/trendlog.c>
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_alarm_ack.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_awf.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_create_object.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_delete_object.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_gas_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_list_element.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_list_element.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_lso.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ts.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ucov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_upt.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_abort.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ack_alarm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_arfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_awfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_dcc.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_error.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_event.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_getevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_lso.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rd.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_readrange.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ts.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_uevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_upt.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whoami.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_youare.c
    )

#
# add ports
#
set(BACNETSTACK_PORT_SRCS
    $<$<BOOL:${CONFIG_BOARD_NATIVE_SIM}>:${BACNETSTACK_POSIX}/bacfile-posix.c>
)

message(STATUS "BACNETSTACK: building for Zephyr")

list(
  APPEND BACNETSTACK_SRCS
      ${BACNETSTACK_BASIC_SRCS}
      ${BACNETSTACK_PORT_SRCS}
)

zephyr_include_directories(include)
add_subdirectory(subsys)

#
# library
#

zephyr_library()

zephyr_library_compile_definitions(
  BACNET_STACK_STATIC_DEFINE
)

zephyr_library_sources(
  ${BACNETSTACK_SRCS}
  )

zephyr_include_directories(
  ${BACNETSTACK_PORT}
  ${BACNETSTACK_SRC}
  $<$<BOOL:${CONFIG_BOARD_NATIVE_SIM}>:${BACNETSTACK_POSIX}>
  )

zephyr_compile_definitions(
  BACNET_CONFIG_H=1
  BACNET_VENDOR_ID=${CONFIG_BACNET_VENDOR_IDENTIFIER}
  BACNET_VENDOR_NAME="${CONFIG_BACNET_VENDOR_NAME}"
  BACNET_PROTOCOL_REVISION=${CONFIG_BACNET_PROTOCOL_REVISION}
  BACNET_STACK_DEPRECATED_DISABLE=1
  $<$<BOOL:${CONFIG_BZLL_NODE_TIME_TO_LIVE_S}>:BZLL_NODE_TIME_TO_LIVE_S=${CONFIG_BZLL_NODE_TIME_TO_LIVE_S}>
  $<$<BOOL:${CONFIG_BZLL_SCAN_NODES_INTERVAL_S}>:BZLL_SCAN_NODES_INTERVAL_S=${CONFIG_BZLL_SCAN_NODES_INTERVAL_S}>
  # datalink API
  $<$<BOOL:${CONFIG_BACDL_NONE}>:BACDL_NONE>
  $<$<BOOL:${CONFIG_BACDL_ALL}>:BACAPP_ALL>
  $<$<BOOL:${CONFIG_BACDL_BSC}>:BACDL_BSC>
  $<$<BOOL:${CONFIG_BACDL_BIP}>:BACDL_BIP>
  $<$<BOOL:${CONFIG_BACDL_BIP_PORT}>:BACDL_BIP_PORT=${CONFIG_BACDL_BIP_PORT}>
  $<$<BOOL:${CONFIG_MAX_BBMD_ENTRIES}>:MAX_BBMD_ENTRIES=${CONFIG_MAX_BBMD_ENTRIES}>
  $<$<BOOL:${CONFIG_MAX_FD_ENTRIES}>:MAX_FD_ENTRIES=${CONFIG_MAX_FD_ENTRIES}>
  $<$<BOOL:${CONFIG_BACDL_BIP6}>:BACDL_BIP6>
  $<$<BOOL:${CONFIG_BACDL_ZIGBEE}>:BACDL_ZIGBEE>
  $<$<BOOL:${CONFIG_BACDL_ARCNET}>:BACDL_ARCNET>
  $<$<BOOL:${CONFIG_BACDL_MSTP}>:BACDL_MSTP>
  $<$<BOOL:${CONFIG_BACDL_ETHERNET}>:BACDL_ETHERNET>
  $<$<BOOL:${CONFIG_BACDL_CUSTOM}>:BACDL_CUSTOM>
  # library features
  $<$<BOOL:${CONFIG_BACNET_BASIC_OBJECTS}>:BACNET_BASIC_OBJECTS>
  $<$<BOOL:${CONFIG_BACNET_BASIC_BACKUP_RESTORE}>:BACNET_BACKUP_RESTORE>
  $<$<BOOL:${CONFIG_BACNET_PROPERTY_LISTS}>:BACNET_PROPERTY_LISTS=1>
  $<$<BOOL:${CONFIG_BACNET_ROUTING}>:BACNET_ROUTING>
  $<$<BOOL:${CONFIG_BACAPP_PRINT_ENABLED}>:BACAPP_PRINT_ENABLED=1>
  $<$<BOOL:${CONFIG_BACAPP_SNPRINTF_ENABLED}>:BACAPP_SNPRINTF_ENABLED=1>
  $<$<BOOL:${CONFIG_BACNET_ADDRESS_CACHE_FILE}>:BACNET_ADDRESS_CACHE_FILE=1>
  MAX_APDU=${CONFIG_BACNET_MAX_APDU_SIZE}
  MAX_TSM_TRANSACTIONS=${CONFIG_BACNET_MAX_TSM_TRANSACTIONS}
  MAX_ADDRESS_CACHE=${CONFIG_BACNET_MAX_ADDRESS_CACHE}
  MAX_CHARACTER_STRING_BYTES=${CONFIG_BACNET_MAX_CHARACTER_STRING_BYTES}
  MAX_OCTET_STRING_BYTES=${CONFIG_BACNET_MAX_OCTET_STRING_BYTES}
  BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE=${CONFIG_BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE}
  $<$<BOOL:${CONFIG_BACNET_BASIC_COV_SUBSCRIPTIONS_SIZE}>:MAX_COV_SUBCRIPTIONS=${CONFIG_BACNET_BASIC_COV_SUBSCRIPTIONS_SIZE}>
  $<$<BOOL:${CONFIG_BACNET_BASIC_COV_ADDRESSES_SIZE}>:MAX_COV_ADDRESSES=${CONFIG_BACNET_BASIC_COV_ADDRESSES_SIZE}>
  # BACnet data types supported for WriteProperty: all = minimal + extra
  $<$<BOOL:${CONFIG_BACAPP_ALL}>:BACAPP_ALL>
  $<$<BOOL:${CONFIG_BACAPP_MINIMAL}>:BACAPP_MINIMAL>
  $<$<BOOL:${CONFIG_BACAPP_TYPES_EXTRA}>:BACAPP_TYPES_EXTRA>
  # BACnet data types supported for WriteProperty: any individual overrides all
  $<$<BOOL:${CONFIG_BACAPP_NULL}>:BACAPP_NULL>
  $<$<BOOL:${CONFIG_BACAPP_BOOLEAN}>:BACAPP_BOOLEAN>
  $<$<BOOL:${CONFIG_BACAPP_UNSIGNED}>:BACAPP_UNSIGNED>
  $<$<BOOL:${CONFIG_BACAPP_SIGNED}>:BACAPP_SIGNED>
  $<$<BOOL:${CONFIG_BACAPP_REAL}>:BACAPP_REAL>
  $<$<BOOL:${CONFIG_BACAPP_DOUBLE}>:BACAPP_DOUBLE>
  $<$<BOOL:${CONFIG_BACAPP_OCTET_STRING}>:BACAPP_OCTET_STRING>
  $<$<BOOL:${CONFIG_BACAPP_CHARACTER_STRING}>:BACAPP_CHARACTER_STRING>
  $<$<BOOL:${CONFIG_BACAPP_BIT_STRING}>:BACAPP_BIT_STRING>
  $<$<BOOL:${CONFIG_BACAPP_ENUMERATED}>:BACAPP_ENUMERATED>
  $<$<BOOL:${CONFIG_BACAPP_DATE}>:BACAPP_DATE>
  $<$<BOOL:${CONFIG_BACAPP_TIME}>:BACAPP_TIME>
  $<$<BOOL:${CONFIG_BACAPP_OBJECT_ID}>:BACAPP_OBJECT_ID>
  $<$<BOOL:${CONFIG_BACAPP_DATETIME}>:BACAPP_DATETIME>
  $<$<BOOL:${CONFIG_BACAPP_DATERANGE}>:BACAPP_DATERANGE>
  $<$<BOOL:${CONFIG_BACAPP_LIGHTING_COMMAND}>:BACAPP_LIGHTING_COMMAND>
  $<$<BOOL:${CONFIG_BACAPP_XY_COLOR}>:BACAPP_XY_COLOR>
  $<$<BOOL:${CONFIG_BACAPP_COLOR_COMMAND}>:BACAPP_COLOR_COMMAND>
  $<$<BOOL:${CONFIG_BACAPP_WEEKLY_SCHEDULE}>:BACAPP_WEEKLY_SCHEDULE>
  $<$<BOOL:${CONFIG_BACAPP_CALENDAR_ENTRY}>:BACAPP_CALENDAR_ENTRY>
  $<$<BOOL:${CONFIG_BACAPP_SPECIAL_EVENT}>:BACAPP_SPECIAL_EVENT>
  $<$<BOOL:${CONFIG_BACAPP_HOST_N_PORT}>:BACAPP_HOST_N_PORT>
  $<$<BOOL:${CONFIG_BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE}>:BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE>
  $<$<BOOL:${CONFIG_BACAPP_DEVICE_OBJECT_REFERENCE}>:BACAPP_DEVICE_OBJECT_REFERENCE>
  $<$<BOOL:${CONFIG_BACAPP_OBJECT_PROPERTY_REFERENCE}>:BACAPP_OBJECT_PROPERTY_REFERENCE>
  $<$<BOOL:${CONFIG_BACAPP_DESTINATION}>:BACAPP_DESTINATION>
  $<$<BOOL:${CONFIG_BACAPP_BDT_ENTRY}>:BACAPP_BDT_ENTRY>
  $<$<BOOL:${CONFIG_BACAPP_FDT_ENTRY}>:BACAPP_FDT_ENTRY>
  $<$<BOOL:${CONFIG_BACAPP_ACTION_COMMAND}>:BACAPP_ACTION_COMMAND>
  $<$<BOOL:${CONFIG_BACAPP_SCALE}>:BACAPP_SCALE>
  $<$<BOOL:${CONFIG_BACAPP_SHED_LEVEL}>:BACAPP_SHED_LEVEL>
  $<$<BOOL:${CONFIG_BACAPP_ACCESS_RULE}>:BACAPP_ACCESS_RULE>
  $<$<BOOL:${CONFIG_BACAPP_CHANNEL_VALUE}>:BACAPP_CHANNEL_VALUE>
  $<$<BOOL:${CONFIG_BACAPP_TIMER_VALUE}>:BACAPP_TIMER_VALUE>
  $<$<BOOL:${CONFIG_LOG_RECORD}>:BACAPP_LOG_RECORD>
  $<$<BOOL:${CONFIG_BACAPP_SECURE_CONNECT}>:BACAPP_SECURE_CONNECT>
  # BACnet data types supported for BACnetChanneValue
  # enabling any individual overrides the NUMERIC defaults
  $<$<BOOL:${CONFIG_CHANNEL_NUMERIC}>:CHANNEL_NUMERIC>
  $<$<BOOL:${CONFIG_CHANNEL_NULL}>:CHANNEL_NULL>
  $<$<BOOL:${CONFIG_CHANNEL_BOOLEAN}>:CHANNEL_BOOLEAN>
  $<$<BOOL:${CONFIG_CHANNEL_UNSIGNED}>:CHANNEL_UNSIGNED>
  $<$<BOOL:${CONFIG_CHANNEL_SIGNED}>:CHANNEL_SIGNED>
  $<$<BOOL:${CONFIG_CHANNEL_REAL}>:CHANNEL_REAL>
  $<$<BOOL:${CONFIG_CHANNEL_DOUBLE}>:CHANNEL_DOUBLE>
  $<$<BOOL:${CONFIG_CHANNEL_OCTET_STRING}>:CHANNEL_OCTET_STRING>
  $<$<BOOL:${CONFIG_CHANNEL_CHARACTER_STRING}>:CHANNEL_CHARACTER_STRING>
  $<$<BOOL:${CONFIG_CHANNEL_BIT_STRING}>:CHANNEL_BIT_STRING>
  $<$<BOOL:${CONFIG_CHANNEL_ENUMERATED}>:CHANNEL_ENUMERATED>
  $<$<BOOL:${CONFIG_CHANNEL_DATE}>:CHANNEL_DATE>
  $<$<BOOL:${CONFIG_CHANNEL_TIME}>:CHANNEL_TIME>
  $<$<BOOL:${CONFIG_CHANNEL_OBJECT_ID}>:CHANNEL_OBJECT_ID>
  $<$<BOOL:${CONFIG_CHANNEL_LIGHTING_COMMAND}>:CHANNEL_LIGHTING_COMMAND>
  $<$<BOOL:${CONFIG_CHANNEL_XY_COLOR}>:CHANNEL_XY_COLOR>
  $<$<BOOL:${CONFIG_CHANNEL_COLOR_COMMAND}>:CHANNEL_COLOR_COMMAND>
  # BACnet data types supported for BACnetTimerStateChangeValue
  # enabling any individual overrides the NUMERIC defaults
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_ALL}>:BACNET_TIMER_VALUE_ALL>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_NUMERIC}>:BACNET_TIMER_VALUE_NUMERIC>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_NULL}>:BACNET_TIMER_VALUE_NULL>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_BOOLEAN}>:BACNET_TIMER_VALUE_BOOLEAN>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_UNSIGNED}>:BACNET_TIMER_VALUE_UNSIGNED>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_SIGNED}>:BACNET_TIMER_VALUE_SIGNED>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_REAL}>:BACNET_TIMER_VALUE_REAL>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_DOUBLE}>:BACNET_TIMER_VALUE_DOUBLE>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_OCTET_STRING}>:BACNET_TIMER_VALUE_OCTET_STRING>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_CHARACTER_STRING}>:BACNET_TIMER_VALUE_CHARACTER_STRING>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_BIT_STRING}>:BACNET_TIMER_VALUE_BIT_STRING>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_ENUMERATED}>:BACNET_TIMER_VALUE_ENUMERATED>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_DATE}>:BACNET_TIMER_VALUE_DATE>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_TIME}>:BACNET_TIMER_VALUE_TIME>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_OBJECT_ID}>:BACNET_TIMER_VALUE_OBJECT_ID>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_NO_VALUE}>:BACNET_TIMER_VALUE_NO_VALUE>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_DATETIME}>:BACNET_TIMER_VALUE_DATETIME>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_CONSTRUCTED_VALUE}>:BACNET_TIMER_VALUE_CONSTRUCTED_VALUE>
  $<$<BOOL:${CONFIG_BACNET_TIMER_VALUE_LIGHTING_COMMAND}>:BACNET_TIMER_VALUE_LIGHTING_COMMAND>
)
