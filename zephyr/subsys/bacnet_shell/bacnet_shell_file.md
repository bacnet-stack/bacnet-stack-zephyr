<!--
SPDX-FileCopyrightText: Copyright The BACnet Stack Zephyr Contributors
SPDX-License-Identifier: Apache-2.0
-->

# BACnet File Shell

Enable `CONFIG_BACNETSTACK_BACNET_FILE_SHELL` with
`CONFIG_BACNET_BASIC_OBJECT_FILE` to add the `bacnet file` shell group.
The chunk size defaults to 64 bytes and is checked against the configured
shell command buffer.

The commands support stream-access File objects only:

```text
bacnet file list
bacnet file info <instance>
bacnet file config [<index> <object-type> <instance>]
bacnet file read <instance> <offset> [len]
bacnet file write <instance> <offset> <hex>
bacnet file dump <instance> [offset] [len]
bacnet file export <instance>
bacnet file import '<json-chunk>'
bacnet file decode <configured-backup-instance>
```

`list`, `info`, `read`, and `write` print a single JSON object with key/value
pairs, for example `{"instance":1,"offset":0,"length":5,"data":"48656c6c6f",
"eof":true}` for `read`. `list` and `info` include a `pathname` field, which is
empty (`""`) until set for a File object. `read` transfers hex data in bounded
chunks; a short
read reports `"eof":true`, and a host downloader should request the configured
chunk size and stop when that flag is set. `config` with no arguments lists the
Device object's `configuration-files` entries as JSON; with an index (1 to the
configured count), an object type, and an instance, it sets that entry using
`Device_Configuration_File_Set()` after validating the object type is `file`
and the instance exists. `dump` uses xxd-compatible rows that can be piped to
`xxd -r`. `export` prints a JSON object containing file metadata, an IEEE CRC32,
and one chunk entry per line. To import data, pass one entry at a time, including
its instance, offset, and hex data, for example:

```text
bacnet file import '{"instance":1,"offset":0,"data":"48656c6c6f"}'
```

`decode` is read-only and accepts only instances configured in the Device
object's `configuration-files` property. It inspects the backup format produced
by `Device_Start_Backup()`: concatenated CreateObject service-request parameter
encodings, with no APDU header and no per-record length prefix. It reports the
file offset of malformed, truncated, or unsupported records and never executes
the decoded requests.

To generate a backup file, use `bacnet reinit startbackup` (note: no hyphen,
unlike most other `bacnet reinit` state names) to trigger `Device_Start_Backup()`,
which writes CreateObject records into the configured backup file. Afterward,
`bacnet file decode <configured-backup-instance>` or `bacnet file export
<configured-backup-instance>` can be used to inspect the result.

Each decoded record is printed as one JSON Lines object. `initial_values` is an
array containing each property identifier and name, array index, priority, and
typed value data. Primitive BACnet values include their tag type and JSON value;
for example:

```json
{"offset":0,"object_type":0,"object_type_name":"analog-input","object_instance":12,"initial_values":[{"property_identifier":85,"property_name":"present-value","array_index":null,"priority":null,"value":[{"type":"real","value":12.5}]}]}
```

Character strings include their encoding. Octet strings are represented as hex,
bit strings as a bit sequence, and date/time wildcard fields as `null`. Values
using context-specific or otherwise unsupported encodings are retained as an
`encoded` value with `data_hex`.

Record-access File objects are not supported. There is no `truncate` command:
the POSIX backend's `bacfile_file_size_set` callback is currently unimplemented,
so a shared truncation command would fail on native/POSIX builds. Offset writes
may leave trailing data when replacing a file with shorter content.
