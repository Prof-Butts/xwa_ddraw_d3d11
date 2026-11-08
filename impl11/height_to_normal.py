"""
Makes tangent-space normal maps from height maps.

The normal maps use the same conventions as the ones made by upscale_textures.py: green
is up (OpenGL style) unless --normal-green-down is given, and the slopes are computed
the same way, so maps from both scripts can be mixed.

Black in the height map is low and white is high (use --invert for the opposite). The
height difference between black and white is --height-range, in pixels of the height
map. 8-bit, 16-bit and color images are accepted; color images are converted to
grayscale.

To get the same normal map as upscale_textures.py from its NAME_height.png, use
--height-range = (groove depth x scale). With that script's defaults (groove depth 0.5,
scale 4), that's 2, which is the default here.

Usage:
    python height_to_normal.py INPUT [INPUT ...] [-o OUTPUT_DIR] [options]

INPUT can be image files or directories. Directories are processed in batch, and the
output keeps the same folder structure. The output is NAME_normal.png, where NAME is the
input's name without a "_height" suffix. Without -o, each normal map is saved next to
its height map.

Requires numpy, scipy and Pillow.
"""
import argparse
import os
import sys
import time

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from upscale_textures import height_to_normals, to_image, find_images  # noqa: E402

DEFAULT_EXTENSIONS = ".png,.bmp,.jpg,.jpeg,.tga,.tif,.tiff"


def load_height(image):
    """Returns the height map as floats in 0..1."""
    if image.mode in ("I;16", "I;16B", "I;16L", "I"):
        data = np.asarray(image, dtype=np.float32)
        # 16-bit images are opened as I;16 or as 32-bit I, depending on the format
        return data / (65535.0 if data.max() > 255 or image.mode.startswith("I;16") else 255.0)
    if image.mode == "F":
        data = np.asarray(image, dtype=np.float32)
        return (data - data.min()) / max(float(data.max() - data.min()), 1e-6)
    if image.mode not in ("L", "LA"):
        rgb = np.asarray(image.convert("RGB"), dtype=np.float32) / 255.0
        return rgb @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
    return np.asarray(image.convert("L"), dtype=np.float32) / 255.0


def output_name(relative):
    stem = os.path.splitext(relative)[0]
    if stem.lower().endswith("_height"):
        stem = stem[:-len("_height")]
    return stem + "_normal.png"


def main():
    parser = argparse.ArgumentParser(description="Make tangent-space normal maps from height maps.")
    parser.add_argument("inputs", nargs="+", help="Height map files or directories")
    parser.add_argument("-o", "--output",
                        help="Output directory. The folder structure of the inputs is kept. Without it, each "
                             "normal map is saved next to its height map")
    parser.add_argument("--height-range", type=float, default=2.0,
                        help="Height difference between black and white, in pixels of the height map "
                             "(default: %(default)s)")
    parser.add_argument("--normal-strength", type=float, default=1.0,
                        help="Multiplies all the slopes in the normal map (default: %(default)s)")
    parser.add_argument("--invert", action="store_true", help="Black is high and white is low")
    parser.add_argument("--normal-green-down", action="store_true",
                        help="Store the down-facing component in green (DirectX style). By default green "
                             "stores the up-facing component (OpenGL style), which is what XWA uses")
    parser.add_argument("--recursive", action="store_true", help="Also process subdirectories")
    parser.add_argument("--extensions", default=DEFAULT_EXTENSIONS,
                        help="Comma-separated extensions to process in directories (default: %s)" % DEFAULT_EXTENSIONS)
    parser.add_argument("--overwrite", action="store_true", help="Replace existing output files")
    args = parser.parse_args()

    extensions = {e.strip().lower() if e.strip().startswith(".") else "." + e.strip().lower()
                  for e in args.extensions.split(",") if e.strip()}

    processed = skipped = failed = 0
    start = time.time()
    for path, relative in find_images(args.inputs, extensions, args.recursive):
        # Don't treat the normal maps made by an earlier run as height maps
        if os.path.splitext(path)[0].lower().endswith("_normal"):
            continue
        root = args.output if args.output else os.path.dirname(path)
        out_path = os.path.join(root, output_name(relative if args.output else os.path.basename(path)))
        if os.path.exists(out_path) and not args.overwrite:
            print("Skipped (exists, use --overwrite): " + out_path)
            skipped += 1
            continue
        try:
            t = time.time()
            with Image.open(path) as image:
                image.load()
                height = load_height(image)
            if args.invert:
                height = 1 - height
            normals = height_to_normals(height * args.height_range, args.normal_strength,
                                        not args.normal_green_down)
            os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
            to_image(normals, "RGB").save(out_path)
            print("%s -> %s (%dx%d, %.1f s)" % (path, out_path, height.shape[1], height.shape[0], time.time() - t))
            processed += 1
        except Exception as e:
            print("FAILED: %s: %s" % (path, e), file=sys.stderr)
            failed += 1

    print("Done: %d processed, %d skipped, %d failed in %.1f s" % (processed, skipped, failed, time.time() - start))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
