#!/usr/bin/env python3
#
# Store each application icon as an exact SVG counterpart of its GIF.
#
# The icons are 16x16 pixel art: there is no smooth curve to recover, so the
# SVG is a pixel grid of merged horizontal runs, one path per colour.  NanoSVG
# rasterizes it back to exactly the GIF pixels at the natural size, and an
# integer scale (2x, 3x, ...) produces exactly the nearest-neighbour enlarged
# pixels; nothing is interpolated or blurred.  This is not a redrawn vector
# logo, which would change the picture.
#
#   python3 scripts/gif2svg.py fox2/apps/pathfinder/*.gif
#   python3 scripts/gif2svg.py --check fox2/apps/*/*.gif
#
# The output is written next to the GIF as <stem>.svg.  --check verifies that
# every SVG has a GIF of the same name and reports what would be written.
#
# Needs Pillow, like the screenshot scripts.
#
import os
import sys
from PIL import Image


def svg_of(source):
    image = Image.open(source).convert('RGBA')
    width, height = image.size
    pixels = image.load()

    # colour -> list of (x, y, length) horizontal runs
    runs = {}
    for y in range(height):
        x = 0
        while x < width:
            colour = pixels[x, y]
            if colour[3] == 0:
                x += 1
                continue
            start = x
            while x < width and pixels[x, y] == colour:
                x += 1
            runs.setdefault(colour, []).append((start, y, x - start))

    out = ['<?xml version="1.0" encoding="UTF-8"?>\n',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
           'viewBox="0 0 %d %d" shape-rendering="crispEdges">\n'
           % (width, height, width, height)]
    for (red, green, blue, alpha), spans in runs.items():
        path = ''.join('M%d %dh%dv1h-%dz' % (x, y, n, n) for x, y, n in spans)
        fill = '#%02x%02x%02x' % (red, green, blue)
        opacity = '' if alpha == 255 else ' fill-opacity="%.3f"' % (alpha / 255.0)
        out.append('<path fill="%s"%s d="%s"/>\n' % (fill, opacity, path))
    out.append('</svg>\n')
    return ''.join(out)


def main(argv):
    check = False
    files = []
    for arg in argv[1:]:
        if arg == '--check':
            check = True
        elif arg in ('-h', '--help'):
            print(__doc__)
            return 0
        else:
            files.append(arg)
    if not files:
        print('usage: gif2svg.py [--check] <icon.gif> ...', file=sys.stderr)
        return 2

    changed = 0
    missing = 0
    for gif in files:
        if not gif.lower().endswith('.gif'):
            print('skip (not a GIF): %s' % gif, file=sys.stderr)
            continue
        svg = gif[:-4] + '.svg'
        if check:
            if os.path.exists(svg):
                print('ok: %s' % svg)
            else:
                print('missing: %s' % svg, file=sys.stderr)
                missing += 1
            continue
        data = svg_of(gif)
        old = None
        if os.path.exists(svg):
            with open(svg, 'r') as f:
                old = f.read()
        if old != data:
            with open(svg, 'w') as f:
                f.write(data)
            changed += 1
            print('wrote %s (%d bytes)' % (svg, len(data)))
        else:
            print('unchanged %s' % svg)

    if check:
        return 1 if missing else 0
    print('%d SVG icon(s) written or updated' % changed)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
