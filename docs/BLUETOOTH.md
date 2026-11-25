# Bluetooth audio

A2DP source with AVRCP, ported from
[thiagoelg/h2-rockbox-bt](https://github.com/thiagoelg/h2-rockbox-bt)
(Thiago Lugli), which builds on Skye's Bluedroid port. The 19 code commits from
`4e3333fd06` through `bde005189c` were cherry-picked with `-x`; their README,
GitHub workflow and manual commits were not taken.

## How it works

- **Stack:** Bluedroid, the ESP-IDF 6.0.1 Bluetooth stack, in `lib/bluedroid/`.
  A2DP source with SBC, plus AVRCP — the headphone's play/pause, next and
  previous work.
- **Host side only:** the player's Broadcom BCM4343A1 is driven over UART
  (HCI H4) by `firmware/target/mips/ingenic_x1000/erosqnative/hci-erosqnative.c`,
  with its patchram firmware in `BCM4343A1_*.c`. **Only hardware revision 4
  (hw4) is supported.**
- **Audio path:** a second PCM sink, `PCM_SINK_BLUETOOTH`. `HAVE_BLUETOOTH` and
  `BT_HEAP_SIZE` (256 KiB TLSF heap) are set in `config/erosqnative.h`.
- **App side:** `apps/bluetooth/bluetooth.c` runs the manager thread and keeps
  paired devices in `.rockbox/bt_devices.txt`. It is reached from **Home**, not
  from the settings menu (`apps/menus/bluetooth_menu.c`). With `BT_DEBUG` it
  logs to `/bt_log.txt`.
- **Settings:** `bt_enabled` (off by default), `bt_high_quality`,
  `bt_volume_sync`.
- **Simulator:** `uisimulator/common/hci-sim.c` talks to a serial HCI device on
  the host.

## Trimming the stack

Bluedroid was carved to this configuration (commit *"bluedroid: remove code this
configuration never builds"*): conditionals that are constant across the
firmware, simulator and `BT_DEBUG` builds were resolved and 159 unused files
deleted — 202k → 99k lines of stack code with identical object code. Its six
config headers (`sdkconfig.h`, `bt_user_config.h`, `bluedroid_user_config.h`,
`bt_target.h`, `dyn_mem.h`, `bt_common.h`) were left as they are; to enable a
disabled Bluedroid feature, restore the code from before that commit.

Porting note: `uart_rx_timeout()` was re-applied by hand on top of upstream's
DMA UART TX driver (`af9a81f5f6`), which their branch predates. That driver
keeps one set of state for all UARTs, so only one UART can be in use at a time.

## Cost and status

Firmware text +425 KB. Tested on an H2 (hw4): Bluetooth audio works, and the
controller's sleep mode is enabled (BT_WAKE asserted only while sending), with
the DAC and headphone amp switched off while playing over Bluetooth — see
[newui/POWER.md](newui/POWER.md).
