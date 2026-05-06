# M5Stack Fire — Journal Firmware

Single-purpose journaling firmware for the M5Stack Fire (ESP32). Power on, type
into today's entry over a paired Bluetooth keyboard, browse past entries with
the three hardware buttons. One `.txt` file per day on the SD card.

## Hardware

- M5Stack Fire (ESP32-D0WDQ6, 16 MB flash, 4 MB PSRAM, 320×240 ILI9341 TFT,
  buttons A/B/C, microSD slot)
- A microSD card (FAT32, ≤ 32 GB)
- One of:
  - A **BLE keyboard** (e.g. Apple Magic Keyboard, Logitech K380 in BLE mode,
    Keychron K-series in BLE mode), or
  - An **M5Stack CardKB** unit plugged into the GROVE A (red) port

## Build

This is a [PlatformIO](https://platformio.org/) project.

```sh
pio run                 # compile
pio run -t upload       # flash
pio device monitor      # serial logs
```

## SD card setup

Create a file `/wifi.txt` at the root of the SD card:

```
ssid=MyWifi
pass=hunter2
tz=PST8PDT,M3.2.0,M11.1.0
```

`tz` follows the POSIX TZ format ([list](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)). If the file is missing the firmware boots without WiFi
and uses the last cached time (or marks the day as `UNDATED-N` until time can be
established).

Entries are written to `/journal/YYYY-MM-DD.txt`.

## First boot

1. Insert SD, power on.
2. The device shows the splash, syncs WiFi+NTP (≈8 s), then opens today's
   entry.
3. **Hold Button B at power-on** to enter Bluetooth pairing mode. Put your
   keyboard into pairing mode; pick its name from the list with A/C and confirm
   with B. Subsequent boots reconnect silently.

## Controls

### Hardware buttons (always available)

| State | A | B | C |
|---|---|---|---|
| Editor (today) | Save now | — | Open entry list |
| Entry list | Move up | Open selected | Move down |
| Viewer | Scroll up | Back to list | Scroll down |

### Bluetooth keyboard

Acts like a normal text editor — printable characters, Backspace, Delete,
Enter, Tab, arrow keys, Home/End. `Ctrl-S` forces a save. `Esc` from the
editor opens the entry list; from the list or viewer it goes back.

### M5Stack CardKB

Plug the CardKB into the red (GROVE A) port — it's auto-detected at boot
on I2C address `0x5F`. Both the CardKB and a paired BLE keyboard can be used
at the same time. `Sym` + `S` / `A` / `E` map to Ctrl-S / Ctrl-A / Ctrl-E
(save / line-home / line-end). Arrow keys, Backspace, Enter, Tab and Esc all
work.

### Save behaviour

The buffer auto-saves 2 s after the last keystroke, on Button A, and on any
state transition out of the editor. Saves are atomic (write-tmp → flush →
rename), so a power loss leaves either the prior good file or the new file
intact — never a half-written one.

## Architecture

```
src/
  main.cpp           — Arduino entry point.
  AppState.{h,cpp}   — State machine: Boot → (Pairing) → WifiSync → Editor ↔ EntryList → Viewer.
  Editor.{h,cpp}     — Today's buffer, cursor, word-wrap, autosave.
  Viewer.{h,cpp}     — Read-only entry display with vertical scroll.
  EntryList.{h,cpp}  — Picker for /journal/*.txt, newest first.
  Storage.{h,cpp}    — SD init, atomic save, list/read entries.
  TimeSync.{h,cpp}   — WiFi connect, NTP, NVS-cached epoch, today() / now() strings.
  WifiCreds.{h,cpp}  — Parser for /wifi.txt.
  BTKeyboard.{h,cpp} — BLE HID host: stack init, scan, bond/reconnect, report parsing.
  CardKB.{h,cpp}     — M5Stack CardKB I2C poller (auto-detected at boot).
  Input.h            — Canonical InputKey struct + KEY_* control-key constants.
  hid_keymap.{h,cpp} — HID Usage ID → InputKey code (US layout).
  Ui.{h,cpp}         — Status bar, splash, toast, error, body region constants.
  Buttons.{h,cpp}    — Debounced edge detection wrapping M5.BtnA/B/C.
  Config.h           — Compile-time constants (autosave interval, paths, NVS keys, colors).
```

## Known risks

- **`esp_hidh` linkage.** arduino-esp32 ships the BT controller and Bluedroid
  stack, but `CONFIG_BT_HID_HOST_ENABLED` may be off in some prebuilt libs.
  If `esp_hidh_init` is missing at link time, switch the platformio.ini to
  `framework = arduino, espidf` and add an `sdkconfig.defaults` with
  `CONFIG_BT_HID_HOST_ENABLED=y`.
- **BLE HID host stability** on the original ESP32 has historical quirks
  around bonding / reconnect. If you can't get a reliable BLE connection,
  fall back to BT Classic HID host (`esp_bt_hid_host_*`); the rest of the app
  is unchanged.
- **Keyboard layout.** US-only initially. To switch, edit
  `src/hid_keymap.cpp` only.

## License

MIT (place a `LICENSE` file at the root if you want to publish).
