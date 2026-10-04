# Pocket Wordcards agent instructions

Read the product handoff and preserve the approved character UI and three-key interaction. Keep code under main and reusable hardware changes under components/bsp. Hardware source of truth: bsp_pins.h, public BSP headers, hardware guide.

Required AI skills: passport-develop, passport-setup, passport-build, passport-device-test and passport-debug in skills/. Install missing local links using each skill's instructions. Run tools/validate.sh before delivery; distinguish host, build, flash, device and publish evidence.

Never automatically erase NVS after a read error. Do not flash without explicit approval for the exact image, target and data impact. Follow docs/development/ai-guide.md. Commit, push, GitHub publication and community submission require explicit authorization. Preserve MIT/OFL notices; do not represent unreviewed content/audio as release-ready. Keep independent Git history and never merge upstream commits.
