# Pocket Wordcards / 随身词卡

An offline vocabulary application for FoloToy AI Passport. **Development preview, not a reviewed educational release.** [中文说明](README.zh_CN.md).

- ESP32-C3, 8 MB flash, no PSRAM; 240×320 RGB565 rounded display.
- Original character theme: ivory, wine red, mist aqua and indigo. Physical up/down/OK keys; no touch interaction.
- 3,676 candidate high-school entries with IPA, parts of speech, Chinese definitions and locally generated UK voice audio. No network initialization or runtime TTS.
- Front: OK marks known and advances; DOWN enters relearning; UP pronounces. Relearning plays the again cue then automatically pronounces; OK finishes at any time and schedules the word next round. Short definitions use UP to replay; long definitions use UP/DOWN to page. Pending relearning survives exit/restart and cannot be promoted to known.
- Up to 20 unique cards per round, at most five new cards after due reviews. Successful reviews advance through 1/2/4/8/16-round intervals; retry schedules next round.
- Two CRC-protected NVS snapshots, stable-ID corpus migration, explicit reset; no automatic erase on read errors.
- Backlight sleeps after 60 seconds; first key wakes without rating a card.

## Validation and build

Install ESP-IDF 5.5.3, activate its environment, then run `idf.py build` to resolve pinned components. Run `./tools/validate.sh` for model/store/BSP tests, asset integrity, native LVGL rendering, isolated firmware build, merged-image verification and artifact archiving. Native rendering additionally requires CMake and a host C compiler; MP3 verification requires FFmpeg.

Generated screenshots: `build/ui-preview/`. Validated app-only merged image: `build/FoloToy-AI-Passport-full.bin`; required separate audio image and installation manifest: `build/install/`. Never flatten all three installation images across the protected cardid gap. Debug artifacts and hashes: `build/firmware/`. Build outputs are ignored by Git. A successful host render/build does not establish device behavior.

## Release status

Definitions, IPA and individual recordings have **not** received full editorial/listening review. ECDICT high-school tags are candidate selection, not a verified syllabus list. Data provenance and Piper voice/model license chain must be checked before redistribution. See [release preparation](docs/product/RELEASE.md) and [handoff](docs/product/HANDOFF.md).

This repository was initialized independently with no upstream Git history. Reusable BSP files originate from the MIT-licensed snapshot recorded in [UPSTREAM](UPSTREAM); attribution is retained. The public source repository is https://github.com/pizzanicky/ai-passport-wordcards. Source publication does not indicate a completed firmware release or FoloToy platform submission.

Code: MIT; Noto fonts: OFL; dictionary/model/audio/character assets have separate terms described in [assets](assets/README.md). Do not assume the code license covers all assets. Flashing requires explicit authorization for the target, exact image and data impact; see [device procedure](docs/development/ai-guide.md).

The current developer installer uses three separate flash writes to preserve the official cardid identity region. It is not yet compatible with a community installation flow that requires a single image. Audio is stored locally in its own flash partition and verified against its SHA256 at startup.
