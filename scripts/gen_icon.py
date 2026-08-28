#!/usr/bin/env python3
"""Generates the 10x10 1-bit fap icon for the Credit Card Reader app."""
from PIL import Image

# 10x10 credit card glyph: outline with a magstripe band near the top.
rows = [
    "0000000000",
    "0111111110",
    "0111111110",
    "0111111110",
    "0100000010",
    "0111111110",
    "0100110010",
    "0100110010",
    "0111111110",
    "0000000000",
]

im = Image.new("1", (10, 10), 1)
for y, row in enumerate(rows):
    for x, ch in enumerate(row):
        im.putpixel((x, y), 0 if ch == "1" else 1)

im.save("icon.png")
print("wrote icon.png")
