# MR60BHA2 Radar Kit – Detailed Project Log

## 0. Summary

Goal: understand the firmware of the Seeed MR60BHA2 kit (XIAO ESP32-C6 + 60 GHz radar), make it configurable, and get full use of the radar for our own projects.

What we got done:
1. Installed a reverse-engineering toolchain under `G:\tools`
2. Backed up the full XIAO flash, split it into partitions and decompiled the radar parser
3. Built a configurable ESPHome firmware for Home Assistant (compiled, not flashed)
4. Identified the radar module (Hi-Link HLK-LD6002, Andar ADT6101P chip)
5. Analyzed the radar firmware 1.6.12 in Ghidra: RF config, signal chain, vital-sign algorithm, command dispatcher, all frame types
6. Recovered the official update procedure from the vendor's OTA tool without running it
7. Flashed the XIAO as a transparent USB↔UART bridge
8. Updated the radar firmware from 1.6.3 to 1.6.12 and verified all new features
9. Wrote PC tools: frame parser, live view, capture tool, flash script
10. Measured and decoded the raw-data frame format

---

## 1. Toolchain (`G:\tools`)

| Tool | Version | Location | Purpose |
|---|---|---|---|
| rea (`rea-agents`) | 6.4.0 | `G:\tools\rea` | Reverse-engineering CLI / MCP server |
| Ghidra | 12.1.4 | `G:\tools\ghidra_12.1.4_PUBLIC` | Disassembly/decompilation (RISC-V, ARM Cortex-M) |
| JDK | 21 | system | Required by Ghidra |
| Python venv | 3.13 | `G:\tools\venv` | Python tooling below |
| esptool | 5.5.0 | venv | ESP32 flash read/write |
| ESPHome | 2026.9.1 | venv | Firmware builds for the XIAO |
| ESP-IDF | 5.5.5 | `G:\tools\cache` | Installed by ESPHome (esp32c6 target only) |
| PlatformIO core | – | `G:\tools\platformio` | Build environment |
| pyserial, xmodem 0.4.5 | – | venv | Serial I/O, radar firmware transfer |
| matplotlib, numpy | – | venv | Live view |
| capstone | – | venv | ARM Thumb disassembly |
| xdis | – | venv | Cross-version Python bytecode disassembly |
| pyinstxtractor | – | `G:\tools\pyinstxtractor.py` | Static extraction of PyInstaller executables |
| gdown | – | venv | Google Drive download |

Environment:
- User-level `GHIDRA_INSTALL_DIR=G:\tools\ghidra_12.1.4_PUBLIC`
- `PATH` extended with `G:\tools\rea\node_modules\.bin` and `G:\tools\venv\Scripts`

**ESPHome build notes:**
- Must run from PowerShell. From Git Bash the ESP-IDF installer aborts with "MSys/Mingw is not supported".
- Set `$env:LOCALAPPDATA='G:\tools\cache'`, `$env:PLATFORMIO_CORE_DIR='G:\tools\platformio'` and `$env:ESPHOME_IDF_DEFAULT_TARGETS='esp32c6'` before the build, so the ~GB cache stays on G: and only the C6 target is installed.

**rea limitations found (6.4.0):**
- `analyze`, `decompile` and `inspect-artifact` reject the ESP image ("unsupported binary format").
- After converting the image to ELF they still reject it ("unsupported ELF architecture"). The architecture whitelist in `dist/domain/binaryTarget.js` only covers x86, x86_64, MIPS, ARM and ARM64.
- `inspect-firmware-regions` / `extract-firmware` require Linux and Binwalk.
- `rea doctor` reports Ghidra and Java 21 as healthy.
- Consequence: all analysis was done with Ghidra headless scripts.

---

## 2. Hardware

| Part | Details |
|---|---|
| Controller | Seeed XIAO ESP32-C6 – ESP32-C6FH4 (QFN32) rev 0.1, 4 MB embedded flash, RISC-V 160 MHz, Wi-Fi 6, BLE 5, 802.15.4 |
| USB | Native USB-Serial/JTAG (VID 303A, PID 1001); appears as a COM port |
| Radar module | Seeed MR60BHA2 = rebranded **Hi-Link HLK-LD6002** |
| Radar SoC | **Andar ADT6101P** – ARM Cortex-M3, 2T2R PCB antennas, 58–62 GHz service band (chip 57–64 GHz), ±60° beam |
| Datasheet specs (HLK) | Breathing/heart detection 0.4–1.5 m, breathing 9–48 /min, heart 60–150 bpm, 50 ms refresh, 3.1–3.5 V |
| Radar ↔ XIAO | UART, XIAO GPIO17 = RX, GPIO16 = TX, 115200 baud, 8N1 |
| Light sensor | BH1750 at I²C address 0x23, SDA GPIO22, SCL GPIO23 |
| LED | WS2812 RGB on GPIO1 (GRB order) |
| Radar-board USB-C | Power only. It does not enumerate on the PC, so the XIAO's USB is the only data path. |

---

## 3. XIAO firmware (original)

### Backup
- Full 4 MB dump: `G:\KI\xiao-c6\flash_full.bin` (SHA-256 `C0C3D7FEF24ACA9C0A68BB714EC5B89D32EBBA9EBA8F4A76F8FE4E0912E8F972`)
- Read at 921600 baud in about 26 s, read-only

### Partition table (at 0x8000)

| Name | Type | Offset | Size |
|---|---|---|---|
| otadata | data | 0x009000 | 0x002000 |
| phy_init | data | 0x00B000 | 0x001000 |
| app0 | app (ota_0) | 0x010000 | 0x1C0000 |
| app1 | app (ota_1) | 0x1D0000 | 0x1C0000 (empty) |
| nvs | data | 0x390000 | 0x06D000 |

### App image (app0)
- Stock Seeed firmware, ESPHome 2024.12.2, project `seeedstudio.mr60bha2_kit` v2.0, compiled 2024-12-24
- 5 segments, entry point `0x40801BFE`, checksum and SHA-256 validation hash valid
- Converted to RISC-V ELF (`app0.elf`) for Ghidra
- Ghidra analysis: 4845 functions; parser functions found via the strings `HEAD_CKSUM_FRAME ERROR` and `DATA_CKSUM_FRAME ERROR`
- Decompiled parser (`G:\KI\xiao-c6\ghidra\radar_decomp.c`) is identical in behavior to the public ESPHome component `seeed_mr60bha2`

### Weaknesses found in the stock firmware
1. Frames of type 0x0A13 (phases) are dropped by the validator
2. Every frame is published unfiltered: no smoothing, no plausibility check, no rate limit
3. Values of 0 are ignored, so stale readings stay visible
4. When presence drops, hard 0 values are published, which distorts history and statistics
5. No distance gating

### Configurable firmware (built, not flashed)
`G:\KI\xiao-c6\esphome\mr60bha2-tuned.yaml` + local component `components\seeed_mr60bha2`:

- Component changes:
  - Parses 0x0A13 into three new optional sensors (total, breath and heart phase)
  - New option `absence_unknown` publishes NaN instead of 0
- Runtime settings in Home Assistant / web UI, stored in flash:

| Setting | Default |
|---|---|
| Max detection distance | 150 cm |
| Heart rate plausible | 40–140 bpm |
| Breathing plausible | 6–30 /min |
| Smoothing (EMA α) | 0.3 |
| Presence on delay | 0 s |
| Presence hold time | 30 s |

- Rates are published at most every 2 s (`throttle_with_priority`)
- Original entity names are kept
- Build result: 1.02 MB image, 55.6 % of the app partition, 33 % RAM

---

## 4. Radar firmware analysis (v1.6.12)

Source image: official `MR60BHA2_eeprom_1.6.12.bin` from Seeed (42,356 bytes, SHA-256 `10906046…d284ec3d`, not encrypted).

### Image layout
- 16-byte header: `64 A5 64 A5 | 5A 10 00 00 | 02 00 00 00 | 00 …`
- Followed by a Cortex-M vector table: initial SP `0x20007218`, reset handler `0x80D1`, 48 vectors
- App is linked at `0x8000`; the bootloader occupies the area below
- Loaded into Ghidra as raw binary at `0x8000`, language `ARM:LE:32:Cortex`, with SRAM (`0x20000000`) and peripheral (`0x40000000`) blocks added
- 310 functions after analysis
- Strings: `Andar ADT6101P people counting V…` (boot banner), model ID `HLBR1DJ010509`, TinyFrame error messages, `breath rate = %f`, `heart rate = %f`, DBSCAN messages (`Failed to allocate epsilon neighbours`)
- Floating point is done in software (ARM EABI soft-float routines)

### Main loop (`0xE47C`)
- Init sequence: clocks/GPIO, UART, model-ID print, RF setup, then the banner
- Endless loop with a 3-state machine:
  - State 0: command handling (`0xFCF4`)
  - State 1: either `vitals_task` (`0x100F4`) or the raw-data task (`0x10420`), selected by the flag at `0x20000001`
  - 10 ms delay per pass

### RF / chirp configuration
- `fmcwConfig` (`0xCF90`), called from `0xB2A0` with fstart = 60.0 (float constant at `0xB330`), FM = 3500 MHz, t0 = 96 µs, t1 = 120 µs, t2 = 10 µs, 66 chirps
- Matches the SDK signature `fmcwConfig(float fstart, uint32 fm, uint32 t0, uint32 t1, uint32 t2, uint32 chirpNumber)`
- VCO/PLL calibration (`0xF0E4`): sweeps 16 band settings (register offset +0x188, bits 8–11) until the lock bit is set, then uses the middle of the locking range
- Frame timer: 50 ms → 20 Hz
- Signal chain per frame:
  - 256-point range FFT
  - 32 chirps × 120 range bins
  - 2 RX channels
  - Then clustering/tracking of up to 3 targets (each target struct 0x68 bytes)

### Vital-sign algorithm (`vitals_process_and_report`, `0x9C6C`)
- Input: 30 range bins of I/Q around the target → phase
- Breathing: 4th-order IIR band-pass → estimator `0xBCF0`
- Heart: 8th-order IIR band-pass → estimator `0xD814`
- Heart-rate estimator:
  - 128-sample ring buffer, 256-point FFT, peak search in bins 11–30 (≈ 52–140 bpm at 20 Hz)
  - Amplitude threshold 100 (below → invalid), 4-frame warm-up
  - Plausibility tracking over the last 5 estimates
- Output smoothing (double constants in the literal pool):
  - Breathing: `new = 0.95·old + 0.05·raw` (constants at `0x9F2C`/`0x9F30`/`0x9F38`)
  - Heart: `new = 0.9·old + 0.1·raw` (constants at `0x9F54`/`0x9F58`/`0x9F5C`)
- Vital signs are only computed when the target tracker reports a stable target in close range

### Command dispatcher (host → radar, `0xFCF4`)

| Type | Payload | Effect |
|---|---|---|
| `0xFFFF` | – | Version reply (type 0xFFFF, 4 bytes) |
| `0x0A12` | u32 | Echo, reply type 0x0A20 with the same value |
| `0x0A13` | u32 | If ≠ 0 sets flag at `0x20000011` (meaning unknown) |
| `0x0A14` | – | Raw-data mode on (flag `0x20000001` = 1) |
| `0x0A15` | – | Raw-data mode off |
| `0x0A16` | u32 | Zone minimum distance, only accepted if < maximum |
| `0x0A17` | u32 | Zone maximum distance, only accepted if > minimum |
| `0x0A18` | u32 | Zone width |
| `0x0A19` | – | Zone reset to `{0, 1500, 1500}` |
| `0x0A20` | – | Read zone config, reply type 0x0A28 (3 × u16) |
| `0x3000` | – | Write update flag to flash, ack 0x3000, reboot into bootloader (**dangerous**) |
| `0x4000` | – | Write boot-mode flag to flash, reboot (**dangerous**) |

Zone notes:
- The struct sits at `0x2000001A` (3 × u16, cm)
- Values are divided by 100.0 and compared against target x/y in metres; the width is applied as ±width/2 sideways
- Settings are RAM-only and lost on restart

### Frames radar → host

| Type | Size | Content |
|---|---|---|
| `0x0A04` | 4 + 16·n | Point cloud: i32 count, then per target {f32 x, f32 y, i32 value, u32 id} |
| `0x0A13` | 12 | Phase: f32 total, f32 breath, f32 heart |
| `0x0A14` | 4 | Breathing rate (f32, /min) |
| `0x0A15` | 4 | Heart rate (f32, bpm) |
| `0x0A16` | 8 | Distance: u32 flag, f32 cm |
| `0x0A17` | 8 | Target position: f32 x, f32 y (m) |
| `0x0A20` | 4 | Echo reply |
| `0x0A28` | 6 | Zone config: u16 min, max, width (cm) |
| `0x0A29` | 2 | Target status (u16) |
| `0x0A32` | 1024 | Raw data RX1 |
| `0x0A34` | 1024 | Raw data RX2 |
| `0x0F09` | 2 | Presence (u16) |
| `0x0100` | var. | Debug text (seen in 1.6.3, e.g. `" NO target!!! "`) |
| `0x3000` | 1 | Update request acknowledged |
| `0xFFFF` | 4 | Version (e.g. `04 01 06 0C` = 1.6.12) |

### TinyFrame format
```
SOF(0x01) | ID u16 BE | LEN u16 BE | TYPE u16 BE | HEAD_CKSUM | DATA[LEN] (little-endian values) | DATA_CKSUM
checksum = bitwise NOT of XOR over the covered bytes (header: bytes 0..6, data: payload)
```
- Frames without payload end after HEAD_CKSUM
- Host frames use ID 0x8000 in the vendor demo
- Example version request: `01 80 00 00 00 FF FF 7E`

### Raw-data format (measured on the device)
- 1024 bytes = 256 complex int16 values (I, Q interleaved) = **8 × 32 matrix**
- Rows = 8 range steps, columns = 32 Doppler bins, column 0 = stationary objects (DC)
- Columns 1 and 31 are mirror images (±velocity)
- Measured on a capture of 58 frame pairs:
  - DC magnitudes per row (RX1): 1127, 1874, 1977, 458, 367, 407, 878, 292
  - Non-DC (motion) energy was highest in rows 5–6, while the tracker reported a person at about 1.6 m
  - RX1/RX2 phase difference coherence was 1.0 for static rows and dropped to 0.15 in a row with motion
  - Frame-to-frame correlation was 0.96–0.99
- Still open: metres per range step
- Rate in raw mode: about 5 frame pairs per second at 115200 baud

---

## 5. Vendor documents and SDK

Downloaded to `G:\KI\mr60bha2-radar\hlk\` (Hi-Link Google Drive folder for HLK-LD6002):
- Communication protocol PDF (documents only 0x0A13–0x0A16, same as Seeed's)
- Module specification V1.0
- Firmware-upgrade guide: the Andar OTA tool uses 1382400 baud for the app and switches to 115200 for the bootloader, which prints `C`
- **TF_Demo**: Andar SDK for ADT310x/ADT60xx (Keil project, Makefile demo, Python TinyFrame demo)
  - Register maps: `rfc_ctype_map.h` (RF controller), DSP, DMA, PMU, SPI, UART, timer, watchdog, TRNG
  - TX driver functions: `fmcwConfig`, `fmcwConfig_dualSaw`, `tdMimo`, `txPaEnable`, `txPaBias`, `rfPhaseShift`, `plliConfig`, `pllnConfig`, `temperatureCompensation`
  - RX driver functions: `rxLnaEnable`, `lnaRegularBias`, `rfTiaGainSetting`, `rfVgaGainSetting`, `tiaHpfConfig`, `rcHpfConfig`, `pgaLpfConfig`, `pgaHpfConfig`
  - UART: `BAUDDIV = hclk / baud`, hclk 50 or 125 MHz in the SDK (100 MHz in the 1.6.12 image); supported rates listed as 115200 and 1382400
- Upper-computer test software (276 MB RAR, includes `.mcap` recordings) – downloaded, not extracted

Seeed downloads in `G:\KI\mr60bha2-radar\download\`:
- Firmware 1.6.12, `MR60BHA2_OTA.zip` (Seeed OTA.exe), protocol PDF, module spec

---

## 6. Update procedure (recovered from Seeed OTA.exe)

Seeed OTA.exe is a PyInstaller-packed PyQt5 app (Python 3.8, © Andar Technologies). It was extracted statically with pyinstxtractor; the PYZ archive was read with xdis and disassembled. The tool was never executed.

Its logic:
1. "Request update" → `tf_compose(12288)` = TinyFrame type `0x3000`, no payload
2. The radar writes an update flag, acknowledges with 0x3000 and reboots. The bootloader prints e.g. `[Boot Area] isUpdateFg = 0xff, updateAppxFg: 0x4, loadFg: 0x2, factoryFg:0xff` / `[Boot Area] Update App2.` and then repeatedly sends `C`
3. The file is sent unmodified (header included) with `XMODEM(getc, putc).send(stream, retry=16, timeout=60)`: 128-byte blocks, CRC mode, xmodem library 0.4.5

Pitfall: the xmodem library counts every unexpected byte before the first `C` as an error and aborts after 16. Our script therefore waits for three clean `C` in a row, flushes the input buffer, and only then starts XMODEM.

---

## 7. What was flashed

### XIAO: USB↔UART bridge
- Config: `G:\KI\xiao-c6\esphome\mr60bha2-bridge.yaml`, component `components\usb_uart_bridge`
- Behavior: forwards radar UART (GPIO17/16, 115200, 8 KB RX buffer) ↔ USB-Serial-JTAG (8 KB TX buffer) in both directions; logger disabled so the stream stays clean
- Flashed with esptool (`firmware.factory.bin` at 0x0, 294 KB, hash verified)
- First test: 1086 frames in about 7 s, 0 checksum errors

### Radar: 1.6.3 → 1.6.12
- Script: `G:\KI\mr60bha2-radar\flash_radar.py` (`--check` dry run, `--resume` if the bootloader is already waiting)
- Pre-checks: radar answers the version query, file header magic, SP in SRAM range, reset vector inside the image
- Transfer: 331 packets, 0 errors, about 20 s
- Version before: `03 01 06 03`, after: `04 01 06 0C`
- Full log: `G:\KI\mr60bha2-radar\flash_log.txt`

### Differences seen 1.6.3 → 1.6.12

| | 1.6.3 | 1.6.12 |
|---|---|---|
| Frames sent | 0x0A14, 0x0A15, 0x0A16, 0x0100 (debug text) | 0x0A04, 0x0A16, 0x0A17, 0x0A29, 0x0F09 (+ vital signs when a person is close) |
| Zone query (0x0A20) | No answer | Answers 0x0A28 = `0 / 1500 / 1500` |
| Raw-data mode | – | 0x0A32/0x0A34 at about 5 Hz |

---

## 8. PC tools (`G:\KI\mr60bha2-radar\dashboard\`)

| File | State | Purpose |
|---|---|---|
| `dashboard.py` | works | TinyFrame parser (separates frames from debug text), decoders for all known types, frame builder, command table (bootloader commands blocked), SSE server (no web front end) |
| `live.py` | works | matplotlib live window: breathing/heart/distance history, phases, top-down map with targets and point cloud, status (version, zone, presence, frames/s, debug text, counters). Turns raw mode off at startup. Keys: `v` version, `n` raw off, `z` zone, `c` clear |
| `capture.py` | works | Records all frames with timestamps to `.npz`; modes `track`, `raw`, `alt` (switches every 3 s) |
| `radar_io.py` | not yet tested on device | Background serial reader + `raw_to_matrix()` (1024 bytes → 8×32 complex) |
| `raw_scope.py` | incomplete | Fragment, not runnable |
| `aufnahmen\verify1.npz` | – | 24 s alternating capture used for the raw-format analysis |

Start the live window:
```
G:\tools\venv\Scripts\python.exe G:\KI\mr60bha2-radar\dashboard\live.py --port COM3
```

---

## 9. Incidents and lessons

- **"Bricked" scare:** the radar had been left in raw mode, so it sent only 0x0A32/0x0A34 and no vital or tracking frames. The live window looked frozen. Sending `0x0A15` restored normal output immediately, and the version was still 1.6.12. `live.py` now sends `0x0A15` on startup, and the raw-on key was removed.
- The stock ESPHome firmware and the bridge cannot both run on the XIAO, so Home Assistant receives no data while the bridge is installed.
- Ghidra headless cannot open a project the GUI has open (lock). A second project `ghidra-headless\radar_hl` was used for scripted analysis.

---

## 10. Open points

- Reconnect Home Assistant: restore the original (`esptool --port COM3 write-flash 0 G:\KI\xiao-c6\flash_full.bin`) or flash `mr60bha2-tuned.yaml`
- Metres per range step in the raw data
- Meaning of command `0x0A13` and of the `0x0A29` status values
- Header word `0x0000105A` in the firmware image (purpose unknown)
- `raw_scope.py` is incomplete
- Not done: web dashboard, raw-data scope, tracking tools, baud-rate change of the radar firmware

---

## 11. Key paths

| Path | Contents |
|---|---|
| `G:\KI\xiao-c6\` | XIAO flash backup, partitions, strings, ELF, Ghidra project |
| `G:\KI\xiao-c6\esphome\` | ESPHome configs (tuned, bridge), local components, build logs |
| `G:\KI\mr60bha2-radar\download\` | Seeed firmware 1.6.12, OTA tool, extracted/disassembled tool code |
| `G:\KI\mr60bha2-radar\ghidra\` | Ghidra GUI project + decompiled function dumps (`radar_fw_decomp.c`, `vitals_decomp.c`, `tf_send_callers.c`, `main_rx.c`, `setters.c`, `zone.c`, `rf_init.c`, `rf_cfg.c`, `uart.c`) |
| `G:\KI\mr60bha2-radar\ghidra-headless\` | Second Ghidra project for scripted analysis |
| `G:\KI\mr60bha2-radar\hlk\` | Hi-Link documents, Andar SDK (`tfdemo\TF_Demo`) |
| `G:\KI\mr60bha2-radar\dashboard\` | PC tools and recordings |
| `G:\KI\mr60bha2-radar\flash_radar.py`, `flash_log.txt` | Radar flash script and log |
