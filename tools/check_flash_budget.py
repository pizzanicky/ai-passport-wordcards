#!/usr/bin/env python3
"""Keep at least ten percent of the factory partition free."""
import pathlib, sys
size = pathlib.Path(sys.argv[1]).stat().st_size
capacity = 0x1b0000
assert size <= capacity * 0.9, f"Flash reserve below 10%: {size}/{capacity}"
print(f"Flash budget: PASS ({size} bytes; {(capacity-size)/capacity:.1%} free)")

config=pathlib.Path(sys.argv[2]).read_text()
assert "CONFIG_LV_USE_FONT_COMPRESSED=y" in config, "Compressed font decoder must be enabled in actual firmware config"
print("Compressed font support: PASS")
