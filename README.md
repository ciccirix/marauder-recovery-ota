# Marauder Recovery OTA (WiFi flashing for 4 MB ESP32)

A tiny **recovery firmware** that lets you flash your main Marauder (or any big
ESP32 app) **over WiFi**, even on a **4 MB** module where a classic dual‑OTA
scheme doesn't fit.

> ⚡ **Flash it from your browser:** [**ciccirix.github.io/flasher**](https://ciccirix.github.io/flasher)
> — one‑click Web Serial installer (Chrome/Edge), no toolchain needed. Pick
> *Marauder C5* to write the full recovery‑OTA image over USB the first time;
> after that you update over WiFi.

Born on an **ESP32‑C5** whose USB‑data lines died (board still powered, screen
on, but no COM port on either USB‑C connector). With this, you never depend on a
flaky USB port again: open the recovery AP from your phone, upload the `.bin`,
done.

## The problem

Classic OTA needs **two app slots** (running + new). A 2.3 MB Marauder app would
need ~4.6 MB just for the two slots — impossible on a 4 MB module (that's why
those builds use `huge_app`: one 3 MB app, **no OTA slot**).

## The solution: factory recovery + single OTA slot

Instead of two copies of the big app, use:

- a **small `factory` recovery app** (~1 MB) that only does WiFi flashing;
- **one `ota_0` slot** holding the big main app (fits comfortably).

Normal boot runs `ota_0` (your Marauder). To update you jump into `factory`
(the recovery), which receives the new main app over WiFi and writes it to
`ota_0`.

```
0x2000    bootloader
0x8000    partition table (partitions.csv)
0xe000    otadata (boot_app0.bin -> boots ota_0 by default)
0x9000    nvs        (kept: your settings/PIN survive)
0x20000   factory    recovery_ota.ino  (~1 MB)
0x160000  ota_0      your main app     (up to ~2.6 MB)
```

`partitions.csv` here is sized for **4 MB**. On 8/16 MB modules just enlarge
`ota_0` (or use a normal dual‑OTA scheme — you won't need this trick).

## Files

- `recovery_ota.ino` — the recovery firmware. Headless, minimal: SoftAP
  **`Marauder-OTA`** (password `marauder1234`), web uploader at
  **`http://192.168.4.1`**. Writes the uploaded `.bin` to the next OTA slot
  (`ota_0`), sets it as boot, reboots. A **"Boot app"** button exits without
  flashing. Uses the **synchronous `WebServer`** from the core (no `AsyncTCP`
  dependency) — see the note below.
- `partitions.csv` — the `factory` + `ota_0` layout above.
- `enter_recovery.ino.inc` — drop‑in function for **your main app** to jump into
  recovery from a menu (shows AP/URL on screen, then reboots into `factory`).

## Build & flash (once, over USB)

Build the recovery and your main app for your target (Arduino ESP32 core), then:

```bash
# generate the partition binary
python <core>/tools/gen_esp32part.py partitions.csv partitions.bin

# flash everything (adjust bootloader offset: 0x0 on S3/C3, 0x2000 on C5/C6/H2/P4)
esptool --chip <chip> --port <PORT> --baud 460800 write_flash \
  0x2000  <your_bootloader.bin> \
  0x8000  partitions.bin \
  0xe000  <core>/tools/partitions/boot_app0.bin \
  0x20000 recovery_ota.ino.bin \
  0x160000 <your_main_app.bin>
```

`nvs` (0x9000) is not written, so settings/PIN are preserved.

## Using it

1. In your main app add a menu item that calls `enterOtaRecovery()`
   (see `enter_recovery.ino.inc`). It erases `otadata` and reboots → the
   bootloader falls back to `factory` (the recovery).
2. On the phone: join WiFi **`Marauder-OTA`**, open **`http://192.168.4.1`**,
   pick your new `<main_app>.bin`, **Flash**. It reboots into the updated app.
3. If the main app is ever bricked, the bootloader auto‑falls back to `factory`,
   so you can always re‑flash over WiFi.

## Notes

- The recovery is **headless** on purpose (tiny, so it fits `factory`). Your main
  app shows the AP name/URL on screen right before rebooting into it.
- Only the **app** partition content is uploaded — same `.bin` your normal build
  produces.
- **Why the synchronous `WebServer` and not `ESPAsyncWebServer`:** on **ESP‑IDF
  5.x** (Arduino ESP32 core 3.x), an async server backed by `AsyncTCP` can abort
  at boot with `assert failed: tcp_alloc ... Required to lock TCPIP core
  functionality!` — lwIP is built with core‑locking and the TCP PCB gets
  allocated outside the TCPIP thread. The built‑in synchronous `WebServer`
  (WiFiServer/lwIP sockets) avoids that entirely and is plenty for a one‑shot
  uploader, so the recovery uses it. If you insist on the async server, wrap the
  offending calls in `LOCK_TCPIP_CORE()` / `UNLOCK_TCPIP_CORE()` or use an
  `AsyncTCP` build that is core‑locking aware.
- License: MIT. Do whatever, credit appreciated. Not affiliated with the
  official ESP32 Marauder.
