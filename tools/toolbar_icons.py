#!/usr/bin/env python3
"""Builds and checks the Workbench tool bar icons (build/package/DCTelnet/WBIcons).

The icons DCTelnet 1.6 shipped were drawn in 8 colours for the MagicWB palette;
on a standard Workbench pens 4-7 hold other colours and they came out pink and
cyan. The tool bar (OpenToolBarWindow, src/guis.c) draws the icon's planar
images directly, so the images must use only the 4 pens every Workbench has.

The symbols are Lucide icons (ISC, tools/toolbar_icons/LICENSE), rendered with
rsvg-convert: black on grey (pens 1 on 0) normally, white on blue (2 on 3)
pressed. Every byte of the icon other than the two images is kept.

  toolbar_icons.py build [ICONDIR]   render the SVGs into the icons in ICONDIR
  toolbar_icons.py check [ICONDIR]   verify every icon: 2 images, depth 2, whole file parsed
  toolbar_icons.py preview OUT.png [ICONDIR]
"""
import os
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ICONDIR = os.path.join(HERE, "..", "build", "package", "DCTelnet", "WBIcons")
SVGDIR = os.path.join(HERE, "toolbar_icons")

# Icon file -> Lucide symbol.
SYMBOLS = {"connect": "plug-zap", "disconnect": "unplug", "addressbook": "book-user",
           "information": "info", "upload": "upload", "download": "download", "quit": "power"}

PALETTE = [(0xAA, 0xAA, 0xAA), (0x00, 0x00, 0x00), (0xFF, 0xFF, 0xFF), (0x66, 0x88, 0xBB)]
NORMAL = (1, 0)      # symbol pen, background pen
PRESSED = (2, 3)
SYMBOL_SIZE = 24

HEADER = 78          # struct DiskObject as stored
DRAWERDATA = 56      # struct OldDrawerData
IMAGE = 20           # struct Image as stored


class Image:
    def __init__(self, raw, offset):
        (self.left, self.top, self.width, self.height, self.depth, self.dataptr,
         self.pick, self.onoff, self.nextptr) = struct.unpack_from(">hhhhhIBBI", raw, offset)
        self.rowbytes = (self.width + 15) // 16 * 2
        planes = bin(self.pick & ((1 << self.depth) - 1)).count("1")
        self.size = IMAGE + planes * self.height * self.rowbytes
        if offset + self.size > len(raw):
            raise ValueError("image data runs past the end of the file")

    def encode(self, pixels):
        """This image's header with 2 planes holding `pixels` (rows of pens 0-3)."""
        out = bytearray(struct.pack(">hhhhhIBBI", self.left, self.top, self.width, self.height,
                                    2, self.dataptr or 1, 3, 0, self.nextptr))
        for bit in range(2):
            for row in pixels:
                data = bytearray(self.rowbytes)
                for x, pen in enumerate(row):
                    if pen >> bit & 1:
                        data[x >> 3] |= 0x80 >> (x & 7)
                out += data
        return bytes(out)


class Icon:
    """A DiskObject parsed to its last byte (OS 1.3-3.1 layout, no NewIcon/GlowIcon)."""

    def __init__(self, raw):
        self.raw = raw
        magic, version = struct.unpack_from(">HH", raw, 0)
        if magic != 0xE310 or version != 1:
            raise ValueError("not a Workbench icon")
        self.gadget_width, self.gadget_height = struct.unpack_from(">hh", raw, 4 + 8)
        render, select = struct.unpack_from(">II", raw, 4 + 18)
        default_tool, tool_types = struct.unpack_from(">II", raw, 0x32)
        drawer, tool_window = struct.unpack_from(">II", raw, 0x42)
        offset = HEADER + (DRAWERDATA if drawer else 0)
        self.image_start = offset
        self.images = []
        for ptr in (render, select):
            if ptr:
                self.images.append(Image(raw, offset))
                offset += self.images[-1].size
        self.image_end = offset
        if default_tool:
            offset = self._string(offset)
        self.tool_types = []
        if tool_types:
            count = struct.unpack_from(">I", raw, offset)[0] // 4 - 1
            offset += 4
            for _ in range(count):
                end = self._string(offset)
                self.tool_types.append(raw[offset + 4:end].rstrip(b"\0").decode("latin-1"))
                offset = end
        if tool_window:
            offset = self._string(offset)
        revision = struct.unpack_from(">I", raw, 4 + 40)[0] & 0xFF   # gg_UserData
        if drawer and revision == 1:
            offset += 6      # OS 2 NewDrawerData: dd_Flags, dd_ViewModes
        self.end = offset

    def _string(self, offset):
        length = struct.unpack_from(">I", self.raw, offset)[0]
        if offset + 4 + length > len(self.raw):
            raise ValueError("string runs past the end of the file")
        return offset + 4 + length

    def with_images(self, normal, pressed):
        body = self.images[0].encode(normal) + self.images[1].encode(pressed)
        return self.raw[:self.image_start] + body + self.raw[self.image_end:]


def symbol_mask(name):
    """The Lucide symbol as rows of booleans, SYMBOL_SIZE square."""
    png = subprocess.run(["rsvg-convert", "-w", str(SYMBOL_SIZE), "-h", str(SYMBOL_SIZE),
                          os.path.join(SVGDIR, name + ".svg")], check=True, capture_output=True).stdout
    alpha = subprocess.run(["magick", "png:-", "-alpha", "extract", "-depth", "8", "gray:-"],
                           input=png, check=True, capture_output=True).stdout
    return [[alpha[y * SYMBOL_SIZE + x] >= 128 for x in range(SYMBOL_SIZE)] for y in range(SYMBOL_SIZE)]


def compose(mask, width, height, pens):
    fg, bg = pens
    left, top = (width - SYMBOL_SIZE) // 2, (height - SYMBOL_SIZE) // 2
    return [[fg if 0 <= y - top < SYMBOL_SIZE and 0 <= x - left < SYMBOL_SIZE and mask[y - top][x - left]
             else bg for x in range(width)] for y in range(height)]


def build(icondir):
    for icon_name, symbol in SYMBOLS.items():
        path = os.path.join(icondir, icon_name + ".info")
        icon = Icon(open(path, "rb").read())
        if len(icon.images) != 2:
            raise ValueError(path + ": needs a normal and a pressed image")
        w, h = icon.images[0].width, icon.images[0].height
        mask = symbol_mask(symbol)
        out = icon.with_images(compose(mask, w, h, NORMAL), compose(mask, w, h, PRESSED))
        open(path, "wb").write(out)


def pixels(icon, raw, image):
    offset = icon.image_start + (0 if image is icon.images[0] else icon.images[0].size) + IMAGE
    rows = [[0] * image.width for _ in range(image.height)]
    for bit in range(image.depth):
        base = offset + bit * image.height * image.rowbytes
        for y in range(image.height):
            for x in range(image.width):
                if raw[base + y * image.rowbytes + (x >> 3)] & (0x80 >> (x & 7)):
                    rows[y][x] |= 1 << bit
    return rows


def check(icondir):
    """Every tool bar icon parses to its last byte and draws with pens 0-3 only."""
    errors = []
    for icon_name in SYMBOLS:
        path = os.path.join(icondir, icon_name + ".info")
        try:
            icon = Icon(open(path, "rb").read())
            if icon.end != len(icon.raw):
                errors.append("%s: %d bytes after the last field" % (path, len(icon.raw) - icon.end))
            if len(icon.images) != 2:
                errors.append("%s: %d images, want normal + pressed" % (path, len(icon.images)))
            for image in icon.images:
                if image.depth != 2 or image.pick != 3:
                    errors.append("%s: depth %d PlanePick %d, want 2 and 3 (4 Workbench pens)"
                                  % (path, image.depth, image.pick))
            if not icon.tool_types:
                errors.append(path + ": no tool type (the tool bar uses the first as its label)")
        except ValueError as e:
            errors.append("%s: %s" % (path, e))
    for e in errors:
        print(e)
    print("toolbar icons: %s" % ("FAIL" if errors else "ok"))
    return not errors


def write_png(path, rows):
    height, width = len(rows), len(rows[0])
    raw = b"".join(b"\0" + bytes(c for px in row for c in px) for row in rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def preview(out, icondir, scale=3):
    rows = []
    gap = (40, 40, 40)
    for which in (0, 1):
        band = None
        for icon_name in SYMBOLS:
            raw = open(os.path.join(icondir, icon_name + ".info"), "rb").read()
            icon = Icon(raw)
            img = pixels(icon, raw, icon.images[which])
            cells = [[PALETTE[p] for p in row for _ in range(scale)] + [gap] * 6 for row in img]
            band = cells if band is None else [a + b for a, b in zip(band, cells)]
        for row in band:
            rows += [row] * scale
        rows.append([gap] * len(rows[-1]))
    write_png(out, rows)


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("build", "check", "preview"):
        sys.exit(__doc__)
    if sys.argv[1] == "preview":
        preview(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ICONDIR)
        return
    icondir = sys.argv[2] if len(sys.argv) > 2 else ICONDIR
    if sys.argv[1] == "build":
        build(icondir)
    sys.exit(0 if check(icondir) else 1)


if __name__ == "__main__":
    main()
