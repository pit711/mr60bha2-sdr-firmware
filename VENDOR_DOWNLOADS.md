# Vendor files downloaded (MR60BHA2 / HLK-LD6002 project)

All files below were downloaded on 2026-10-11/12 from the manufacturers' public download pages.
They are third-party material (Seeed Studio, Shenzhen Hi-Link, Andar Technologies) and are kept
**locally only** – do not redistribute or upload them.

## 1. Seeed Studio (MR60BHA2 kit)

Source page: https://wiki.seeedstudio.com/getting_started_with_mr60bha2_mmwave_kit/

| File | Download URL | Local path | Size (bytes) | SHA-256 |
|---|---|---|---:|---|
| Radar firmware 1.6.12 | https://files.seeedstudio.com/wiki/mmwave-for-xiao/mr60/firmware/MR60BHA2_eeprom_1.6.12.bin | `G:\KI\mr60bha2-radar\download\MR60BHA2_eeprom_1.6.12.bin` | 42,356 | `10906046140495b54d854b3630a27503c1adb36c614febba8279c854d284ec3d` |
| OTA update tool (Windows, "Seeed OTA.exe", PyInstaller) | https://files.seeedstudio.com/wiki/mmwave-for-xiao/mr60/firmware/MR60BHA2_OTA.zip | `G:\KI\mr60bha2-radar\download\MR60BHA2_OTA.zip` | 31,806,604 | `88bf5aa572710903cec7b9dfd193fd36ed260144c48cedeb73ebbce95105d874` |
| Tiny Frame interface manual (breathing & heartbeat) | https://files.seeedstudio.com/wiki/mmwave-for-xiao/mr60/datasheet/Seeed_Studio_Tiny_Frame_Interface_Breathing_and_Heartbeat.pdf | `G:\KI\xiao-c6\docs\TinyFrame_MR60BHA2.pdf` | 811,735 | `c5753ced60904c21d9c2a03e10887d96131d25a19482dbb49e4ff839fdc4569a` |
| MR60BHA2 module specification | https://files.seeedstudio.com/wiki/mmwave-for-xiao/mr60/datasheet/MR60BHA2_Breathing_and_Heartbeat_Module.pdf | `G:\KI\xiao-c6\docs\MR60BHA2_Spec.pdf` | 1,500,132 | `1a63bdc5e5dc602cfce1023db66a311e62665b5ae57c33b06a14d2c9efd2b41f` |
| Kit ESPHome configuration (open source, GitHub) | https://raw.githubusercontent.com/Seeed-Studio/xiao-esphome-projects/main/projects/seeedstudio-mr60bha2-kit/seeedstudio-mr60bha2-kit.yaml | `G:\KI\xiao-c6\esphome\seeedstudio-mr60bha2-kit.original.yaml` | 1,912 | `5ee07d79f76955068df10c98d19a5359344bfbe4ac16c2767a02e58993cdb81d` |

Mentioned on the wiki but **not** downloaded: GUI software `Seeed_Studio_mmWave_Sensor_MR60BHA2_GUI.zip`.

Derived locally (not a download):
- `G:\KI\mr60bha2-radar\mr60bha2_v1.6.12_app@0x8000.bin` – the 1.6.12 image without its 16-byte header (for Ghidra).
- `G:\KI\mr60bha2-radar\download\ota_exe\`, `seeed_ota_x\`, `pyz_out\` – the OTA tool unpacked statically
  (pyinstxtractor + xdis disassembly) to recover the update procedure; the .exe was never run.

## 2. Shenzhen Hi-Link (HLK-LD6002 = radar inside the MR60BHA2)

Source page: https://www.hlktech.net/index.php?id=1180&cateid=755
Google Drive folder: https://drive.google.com/drive/folders/1oMJYYOvhXx9uJD9SCkCsjMzLqdOg7AqR (downloaded completely with gdown)

| File | Local path (`G:\KI\mr60bha2-radar\hlk\`) | Size (bytes) | SHA-256 |
|---|---|---:|---|
| HLK-LD6002 communication protocol | `HLK-LD6002 breath and heart rhythm Communication protocol.pdf` | 239,799 | `d4439d7367e74f5d0d11be9ef2bdb8c484bc17778ec229776372ecfbefb942ce` |
| HLK-LD6002 module specification V1.0 | `HLK-LD6002 Respiratoryheart rate detection radar module  specifications V1.0.pdf` | 1,108,106 | `3b0df6904f1cac020f8d8edcf4e89fb400f3b12c1f52b7f406d2f57ad68c268c` |
| Firmware upgrade guide | `HLK-6002 firmware upgrade document.pdf` | 941,594 | `a87ea3481302079515015c62f2a7dbe5385a6e15bf1154a79176e37e4899371d` |
| OTA upgrade tool (Andar OTA, Windows) | `HLK-6002 OTA upgrade tool (1).zip` | 41,406,565 | `d01c7e86546907f40c5089733a9a524788bc28dba120259fd6819b0db75d04e1` |
| Kit document HLK-LD60G80G-KIT V2.0 | `HLK-LD60G80G-KIT-V2.0(1).pdf` | 180,396 | `7a6ed531727051bac869dfb997f1cb71896eb3172dc861fbc294ebfd117b1c18` |
| 3D models | `HLK-LD6002 Series -3D photo.zip` | 294,136 | `fe48a88fb229fda7badf7fee61ec6af8d60259fd8b6e602bbe230a6037519cb7` |
| CP2102 USB-serial driver | `CP2102x serial chip driver (1).zip` | 14,336,431 | `69270317d93fc5bd0bee0df594e11e65a23b474fb8572a0e1be7188b28bd30fc` |
| TF_Demo – Andar SDK + TinyFrame demos (C/Keil, Makefile, Python) | `TF_Demo.rar` (extracted to `tfdemo\`) | 21,058,182 | `34272dcbda7b4b2e05b242157c44caa2df061653ad8d167d4ae9c2e21fb7818f` |
| Upper-computer test software (incl. `.mcap` recordings) | `Respiratory and heart rate upper computer testing software.rar` (only listed, not extracted) | 276,656,982 | `da79b5fc00bea3cea7a8b8aaf6e6839377fcde6249adf9fd2d5a1e29c2e925b5` |

The TF_Demo archive contains Andar's ADT310x/ADT60xx SDK (register maps, RF/TX/RX/UART drivers, TinyFrame).
Several Hi-Link documents are marked "Confidential".

## 3. Open-source material used (not vendor files)

Listed for completeness; these are public open-source projects, not manufacturer downloads:
- ESPHome component `seeed_mr60bha2` (limengdu/MR60BHA2_ESPHome_external_components, GitHub) → `G:\KI\xiao-c6\esphome\components\seeed_mr60bha2\` (modified locally).
- ESPARGOS ESP-SDR (`G:\KI\espsdr\esp-sdr`) and the ESPsoup firmware fork (`G:\KI\espsdr\espsoup-firmware`), GPL.
- Tools: Ghidra, rea, ESP-IDF, ESPHome, esptool, pyinstxtractor, xdis, capstone.

## 4. Own device data (not vendor files, private)

- `G:\KI\xiao-c6\flash_full.bin` – full 4 MB backup of the XIAO ESP32-C6 with the original Seeed firmware.
  Its NVS partition can contain Wi-Fi credentials → keep private.
- Backup on the tablet from the installer test: `/sdcard/Download/espsoup-backup-esp32c6-*.bin` → keep private.
