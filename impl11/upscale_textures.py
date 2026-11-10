"""
Upscales textures while making their panel lines thinner.

A plain upscale keeps the lines as wide as before, relative to the texture, so they look
chunky at the higher resolution. This script separates the lines from the rest of the
image, upscales both, makes the lines thinner and puts everything back together:

1. The image is split into brightness (Y) and color (Cb, Cr). The lines are in Y.
2. The lines are extracted from Y with a black-hat transform (closing minus the image),
   which picks up narrow dark grooves and ignores broad shading. A ridge filter keeps only
   line-shaped features, so dirt spots stay in the base image. Dark lines next to narrow
   bright features are kept too: they're the shadows of raised details, not grooves. And
   only lines that are part of large connected structures are thinned.
3. The base (Y with the lines filled in) and the lines are upscaled separately.
4. The lines are made thinner by narrowing their profile, keeping their darkness.
5. The grime is separated from the base: small dark details (dirt, stains, scratches)
   that a median filter removes. What's left is the clean surface.
6. The color is upscaled and sharpened with the brightness as a guide, so color edges
   (like red markings) stay crisp.

The result is put together from three layers that multiply each other:

    output = color x grime x lines

- color: the clean surface, in color.
- grime: white where the surface is clean, darker where there's grime. It's gray, or
  tinted with --grime-color.
- lines: white everywhere except on the (thinned) panel lines.

Each layer's brightness and contrast can be adjusted (--color-*, --grime-*, --lines-*), and
--export-layers saves the three layers so they can be combined by hand: in an image editor,
put grime and lines on top of color with the Multiply blend mode. The color layer is
also saved as a grayscale layer and a chroma layer (hue and saturation only): chroma on
top of gray with the Color blend mode makes the color layer.

--normal-map also makes a tangent-space normal map. Its height map is built from the lines
layer: the thin panel lines become grooves. Optionally, the lighting painted on raised
details can be turned back into shape (a highlight next to a shadow becomes a bump), and
the fine surface texture can be added too. The grime is paint, so it's always left out.
The normal maps and height maps are saved in NormalMaps and HeightMaps subfolders, next
to the upscaled images.

Usage:
    python upscale_textures.py INPUT [INPUT ...] -o OUTPUT_DIR [options]

INPUT can be image files, directories or ZIP files. Directories are processed in batch, and
the output keeps the same folder structure. The output is always PNG.

A ZIP file (like the ones XWA reads DAT images from, with GROUP/IMAGE.png entries) is
extracted to OUTPUT/unzipped, a temporary folder that's deleted afterwards. Its images are
processed into OUTPUT/ZIPNAME, and a new ZIP with the same name as the original is saved in
OUTPUT. The new ZIP only has the processed images (indexed with --indexed), at the same
paths: no normal maps, height maps, layers or folder entries, which XWA would count as
images.

Images named NAME_alpha or NAME_illum are the alpha and illumination maps of the texture
NAME, as XwaOptEditor exports them. They're only resized (with Lanczos), so they still
match their texture. The pixels that a NAME_illum map lights keep the colors of the
original texture (just resized): they're lights, so they look the same in any lighting.
update_opt_textures.ps1 puts the results back into the OPT.

Requires numpy, scipy and Pillow.
"""
import argparse
import os
import shutil
import sys
import time
import zipfile

import numpy as np
from PIL import Image
from scipy import ndimage
from scipy.spatial import cKDTree

DEFAULT_EXTENSIONS = ".png,.bmp,.jpg,.jpeg,.tga"
# Alpha and illumination maps, named like XwaOptEditor exports them. They're only resized.
MAP_SUFFIXES = ("_alpha", "_illum")
# Outputs saved in a subfolder of their image's output folder, so they don't get mixed
# with the textures
LAYER_SUBFOLDERS = {"normal": "NormalMaps", "height": "HeightMaps"}
# ZIP inputs are extracted to this temporary subfolder of the output directory. The marker
# file tells it apart from a folder with the same name that this script didn't make.
UNZIP_DIR = "unzipped"
UNZIP_MARKER = ".upscale_textures_temp"


# ------------------------------------------------------------------------------------
# Color conversion (full-range BT.601 YCbCr, values in 0..1)
# ------------------------------------------------------------------------------------

def rgb_to_ycbcr(rgb):
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    y = 0.299 * r + 0.587 * g + 0.114 * b
    cb = (b - y) * 0.564
    cr = (r - y) * 0.713
    return y, cb, cr


def ycbcr_to_rgb(y, cb, cr):
    r = y + 1.403 * cr
    g = y - 0.344 * cb - 0.714 * cr
    b = y + 1.773 * cb
    return np.stack([r, g, b], axis=-1)


# ------------------------------------------------------------------------------------
# Resampling
# ------------------------------------------------------------------------------------

def resize(channel, width, height):
    """Lanczos resize of a float channel."""
    return np.asarray(Image.fromarray(channel.astype(np.float32), mode="F").resize(
        (width, height), Image.LANCZOS))


# ------------------------------------------------------------------------------------
# Line extraction and thinning
# ------------------------------------------------------------------------------------

def line_likeness(y, sigma, dark):
    """
    0..1: how much each pixel looks like part of a line (a long, narrow valley for dark
    lines, a ridge for bright ones), from the eigenvalues of the Hessian. Spots and flat
    areas get values near 0.
    """
    hxx = ndimage.gaussian_filter(y, sigma, order=(0, 2))
    hyy = ndimage.gaussian_filter(y, sigma, order=(2, 0))
    hxy = ndimage.gaussian_filter(y, sigma, order=(1, 1))
    if not dark:
        hxx, hyy, hxy = -hxx, -hyy, -hxy
    mean = (hxx + hyy) / 2
    delta = np.sqrt(((hxx - hyy) / 2) ** 2 + hxy ** 2)
    across = mean + delta   # Curvature across the line: positive in a valley
    along = mean - delta    # Curvature along the line: near 0 for a line
    likeness = (across - np.abs(along)) / (across + np.abs(along) + 1e-6)
    return np.where(across > 0, np.clip(likeness, 0, 1), 0)


def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / max(edge1 - edge0, 1e-6), 0, 1)
    return t * t * (3 - 2 * t)


def keep_long_structures(weight, min_length):
    """
    Keeps only the parts of the line mask that belong to connected structures at least
    min_length pixels across (the diagonal of their bounding box). Panel lines form long
    connected networks, while the outlines of small details (handles, rivets) are short.
    """
    if min_length <= 0:
        return weight
    labels, count = ndimage.label(weight > 0.25, structure=np.ones((3, 3)))
    if count == 0:
        return weight * 0
    keep = np.zeros(count + 1, dtype=bool)
    for index, box in enumerate(ndimage.find_objects(labels), start=1):
        if box is None:
            continue
        height = box[0].stop - box[0].start
        width = box[1].stop - box[1].start
        keep[index] = (height * height + width * width) >= min_length * min_length
    # Grow the kept structures by a pixel, so they include the faint edges of the lines
    kept = ndimage.binary_dilation(keep[labels], structure=np.ones((3, 3)))
    return weight * kept


def raised_detail_mask(y, line_size, threshold):
    """
    0..1: how close each pixel is to a narrow bright feature, like a raised bar, pin or
    handle. The dark lines next to those are their shadows and outlines, not engraved
    panel lines, so they shouldn't be thinned. Large bright areas (like a whole plate)
    aren't narrow, so the lines around them can still be thinned.
    """
    # Bevel highlights next to grooves are fainter than the highlights on raised parts,
    # and a small window keeps broad light patches from counting as narrow features
    bright = y - ndimage.grey_opening(y, size=line_size + 2)
    # Reach the shadow next to the raised part
    bright = ndimage.maximum_filter(bright, size=line_size)
    return smoothstep(threshold, 1.5 * threshold, bright)


def extract_lines(y, line_size, threshold, shape_filter, min_length, raised_threshold, dark):
    """
    Returns the line layer: how much darker (or brighter) than its surroundings each
    pixel is, for the pixels that are part of lines. Adding it to y (dark lines) or
    subtracting it (bright lines) fills the lines in.
    """
    if dark:
        strength = ndimage.grey_closing(y, size=line_size) - y
    else:
        strength = y - ndimage.grey_opening(y, size=line_size)
    weight = smoothstep(threshold, 2 * threshold, strength)
    if shape_filter > 0:
        likeness = line_likeness(y, sigma=1.0, dark=dark)
        # Spread the likeness a little, so it covers the whole width of the line
        likeness = ndimage.maximum_filter(likeness, size=3)
        weight *= (1 - shape_filter) + shape_filter * likeness
    if raised_threshold > 0 and dark:
        weight *= 1 - raised_detail_mask(y, line_size, raised_threshold)
    weight = keep_long_structures(weight, min_length)
    return strength * weight


def thin_lines(lines, line_width, peak_size, scale):
    """
    Narrows the profile of the lines. Each pixel is compared to the darkness of the core
    of its line, and raising that ratio to a power makes the profile narrower while the
    core keeps its value. For a bell-shaped profile, the power 1 / line_width^2 scales the
    width by line_width.
    """
    if line_width >= 1:
        return lines
    gamma = 1.0 / (line_width * line_width)
    # Remove pixel noise, which the power would turn into dots
    smooth = ndimage.gaussian_filter(lines, sigma=0.35 * scale)
    # The darkness of the core of the line. It's taken from a smoother copy, so it follows
    # the typical darkness along the line instead of its strongest spots, and it's smoothed
    # again so it doesn't change in steps.
    core = ndimage.maximum_filter(ndimage.gaussian_filter(lines, sigma=0.75 * scale), size=peak_size)
    core = ndimage.gaussian_filter(core, sigma=scale)
    ratio = np.clip(smooth / (core + 1e-6), 0, 1)
    return core * ratio ** gamma


# ------------------------------------------------------------------------------------
# Guided color upscaling
# ------------------------------------------------------------------------------------

def guided_filter(guide, source, radius, eps):
    """
    Guided filter (He et al.): smooths source while following the edges of guide. Used to
    snap the upscaled color to the sharp edges of the upscaled brightness.
    """
    size = 2 * radius + 1
    mean_g = ndimage.uniform_filter(guide, size)
    mean_s = ndimage.uniform_filter(source, size)
    cov_gs = ndimage.uniform_filter(guide * source, size) - mean_g * mean_s
    var_g = ndimage.uniform_filter(guide * guide, size) - mean_g * mean_g
    a = cov_gs / (var_g + eps)
    b = mean_s - a * mean_g
    return ndimage.uniform_filter(a, size) * guide + ndimage.uniform_filter(b, size)


# ------------------------------------------------------------------------------------
# Shading removal (--flatten-shading)
# ------------------------------------------------------------------------------------

def estimate_shading(surface, threshold, detail_sigma=2.0):
    """
    The shading painted into a texture (baked ambient occlusion, light gradients), as a log
    factor of the brightness: the texture without it is surface * exp(-shading). Retinex-style:
    in log brightness, painted shading changes slowly (about 1% per pixel), while the edges
    of markings and panels change a lot in a pixel or two. So the gradients sharper than
    threshold (relative change per pixel) are kept as paint, the rest is shading, and the
    paint is rebuilt from its gradients. The fine detail is set aside first (a blur of
    detail_sigma pixels), so it doesn't count as either and isn't flattened. The shading
    averages 0, so the texture keeps its overall brightness.
    """
    log = np.log(surface + 0.01)
    smooth = ndimage.gaussian_filter(log, detail_sigma)
    gx = np.zeros_like(smooth)
    gy = np.zeros_like(smooth)
    gx[:, :-1] = smooth[:, 1:] - smooth[:, :-1]
    gy[:-1, :] = smooth[1:, :] - smooth[:-1, :]
    gx *= smoothstep(threshold, 2 * threshold, np.abs(gx))
    gy *= smoothstep(threshold, 2 * threshold, np.abs(gy))
    shading = smooth - integrate_gradients(gx, gy)
    return shading - shading.mean()


# ------------------------------------------------------------------------------------
# Layer adjustments
# ------------------------------------------------------------------------------------

def adjust_color(rgb, brightness, contrast):
    """Brightness and contrast like in an image editor: contrast pivots around mid-gray."""
    return np.clip((rgb - 0.5) * contrast + 0.5 + brightness, 0, 1)


def adjust_multiply_layer(factor, brightness, contrast):
    """
    Adjusts a multiply layer (white = no effect). Contrast scales how much the layer
    darkens: 0.5 makes it half as dark, 2 twice as dark. Brightness lightens (positive) or
    darkens (negative) the dark parts by a fixed amount, and leaves the white parts white.
    """
    darkness = 1 - factor
    # Only the parts where the layer has something
    weight = smoothstep(0, 0.05, darkness)
    darkness = darkness * contrast - brightness * weight
    return np.clip(1 - darkness, 0, 1)


def tint_multiply_layer(factor, color):
    """
    Turns a grayscale multiply layer into a colored one that darkens toward color, with the
    same brightness: each channel darkens in proportion to how far color is from white in
    that channel. Returns an RGB layer, or factor itself when color is gray or white.
    """
    color = np.asarray(color, dtype=np.float32)
    lum = float(color @ np.array([0.299, 0.587, 0.114], dtype=np.float32))
    if lum >= 0.999 or np.ptp(color) < 1e-3:
        return factor
    weights = (1 - color) / (1 - lum)
    return np.clip(1 - (1 - factor)[..., None] * weights, 0, 1)


# ------------------------------------------------------------------------------------
# Height and normal maps
# ------------------------------------------------------------------------------------

def integrate_gradients(p, q):
    """
    Frankot-Chellappa: the height whose x and y slopes are closest to p and q. The slopes
    are mirrored to twice the size first, so the FFT doesn't wrap the edges around.
    """
    height, width = p.shape
    p = np.block([[p, -p[:, ::-1]], [p[::-1, :], -p[::-1, ::-1]]])
    q = np.block([[q, q[:, ::-1]], [-q[::-1, :], -q[::-1, ::-1]]])
    ky = np.fft.fftfreq(2 * height)[:, None] * 2 * np.pi
    kx = np.fft.fftfreq(2 * width)[None, :] * 2 * np.pi
    denom = kx * kx + ky * ky
    denom[0, 0] = 1
    h = (-1j * kx * np.fft.fft2(p) - 1j * ky * np.fft.fft2(q)) / denom
    h[0, 0] = 0
    return np.real(np.fft.ifft2(h))[:height, :width].astype(np.float32)


def shading_relief(surface, line_size, light_dir):
    """
    Height of raised and sunken shapes, from the lighting painted on them: a slope that
    faces the light is brighter than its surroundings, one that faces away is darker. The
    slope along the light's direction is estimated from that, and integrated into height,
    so a highlight next to a shadow becomes one bump. In pixels of the original image.
    """
    low = ndimage.gaussian_filter(surface, sigma=line_size)
    slope = (surface - low) / np.maximum(low, 0.1)
    # Direction the light travels across the image: the height grows along it on lit slopes
    dx, dy = -light_dir[0], -light_dir[1]
    h = integrate_gradients(slope * dx, slope * dy)
    # Keep only the shapes, not broad tilts across the whole plate
    return h - ndimage.gaussian_filter(h, sigma=2 * line_size)


def height_to_normals(h, strength, green_up):
    """Tangent-space normal map (0..1 RGB) from a height map in pixels of the image."""
    gx = ndimage.sobel(h, axis=1) / 8 * strength
    gy = ndimage.sobel(h, axis=0) / 8 * strength
    # Image Y points down. DirectX-style maps store the normal's down component in green,
    # OpenGL-style (green up) maps store the up component.
    ny = gy if green_up else -gy
    n = np.stack([-gx, ny, np.ones_like(h)], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5


def to_image16(data):
    """16-bit grayscale image, with data stretched to the full range."""
    lo, hi = float(data.min()), float(data.max())
    data = (data - lo) / max(hi - lo, 1e-6)
    return Image.fromarray((data * 65535 + 0.5).astype(np.uint16))


# ------------------------------------------------------------------------------------
# Processing
# ------------------------------------------------------------------------------------

# The luminosity used by the Color and Luminosity blend modes of image editors (the
# non-separable blend modes of the PDF, Photoshop and CSS compositing specs)
BLEND_LUM = np.array([0.3, 0.59, 0.11], dtype=np.float32)


def set_lum(rgb, lum):
    """SetLum from the blend mode specs: the color of rgb at the luminosity lum."""
    c = rgb + (lum - rgb @ BLEND_LUM)[..., None]
    # ClipColor: bring out-of-range colors back in, keeping their luminosity and hue
    l = (c @ BLEND_LUM)[..., None]
    lo = c.min(axis=-1, keepdims=True)
    hi = c.max(axis=-1, keepdims=True)
    c = np.where(lo < 0, l + (c - l) * l / np.maximum(l - lo, 1e-6), c)
    c = np.where(hi > 1, l + (c - l) * (1 - l) / np.maximum(hi - l, 1e-6), c)
    return np.clip(c, 0, 1)


def to_image(data, mode):
    return Image.fromarray((np.clip(data, 0, 1) * 255 + 0.5).astype(np.uint8), mode)


def process_image(image, args, lit=None):
    """
    Returns the upscaled image, the color, grime and lines layers (for --export-layers),
    and the intermediate layers (for --save-layers). lit is the mask of the pixels that the
    texture's illumination map lights, at the output size. They're left as they are in
    the original texture (upscaled): they look the same in any lighting, so there's no
    shading to remove, and the engine makes them bloom by their brightness.
    """
    has_alpha = image.mode in ("RGBA", "LA") or (image.mode == "P" and "transparency" in image.info)
    rgba = np.asarray(image.convert("RGBA")).astype(np.float32) / 255.0
    rgb, alpha = rgba[..., :3], rgba[..., 3]
    height, width = alpha.shape
    out_width, out_height = width * args.scale, height * args.scale

    y, cb, cr = rgb_to_ycbcr(rgb)

    # Separate the lines from the base
    dark_lines = extract_lines(y, args.line_size, args.line_threshold, args.shape_filter,
                               args.min_line_length, args.raised_threshold, dark=True)
    base = y + dark_lines
    bright_lines = None
    if args.thin_highlights:
        bright_lines = extract_lines(y, args.line_size, args.line_threshold, args.shape_filter,
                                     args.min_line_length, 0, dark=False)
        base = base - bright_lines

    # Separate the grime: the dark details the median removes. The clean surface keeps
    # the edges of large shapes, and anything brighter than it stays in the surface too.
    clean = base
    if args.grime_size > 1:
        clean = ndimage.median_filter(base, size=args.grime_size)
        # The shadows of raised details are part of the shape, not grime
        if args.raised_threshold > 0:
            raised = raised_detail_mask(base, args.line_size, args.raised_threshold)
            clean = clean + (base - clean) * raised

    # Upscale and thin the lines. The thinned highlights are part of the surface.
    peak_size = args.line_size * args.scale
    surface_hi = resize(base, out_width, out_height)
    clean_hi = resize(clean, out_width, out_height)
    dark_hi = np.clip(resize(dark_lines, out_width, out_height), 0, None)
    dark_thin = thin_lines(dark_hi, args.line_width, peak_size, args.scale)
    if bright_lines is not None:
        bright_hi = np.clip(resize(bright_lines, out_width, out_height), 0, None)
        bright_thin = thin_lines(bright_hi, args.line_width, peak_size, args.scale)
        surface_hi = surface_hi + bright_thin
        clean_hi = clean_hi + bright_thin
    y_hi = surface_hi - dark_thin

    # Split the brightness into the three multiply layers. Without adjustments,
    # color_y * grime * lines == y_hi.
    color_y = np.maximum(np.maximum(clean_hi, surface_hi), 1e-4)
    grime = np.clip(surface_hi / color_y, 0, 1)
    lines = np.clip(y_hi / np.maximum(surface_hi, 1e-4), 0, 1)

    # Upscale the color, snapped to the edges of the new brightness
    cb_hi = resize(cb, out_width, out_height)
    cr_hi = resize(cr, out_width, out_height)
    if not args.no_guided_chroma:
        radius = max(1, args.scale)
        cb_hi = guided_filter(y_hi, cb_hi, radius, eps=1e-3)
        cr_hi = guided_filter(y_hi, cr_hi, radius, eps=1e-3)
    # The color layer is the surface before the grime and lines darken it, so its color
    # has to be as strong as the brightness it gets: undo the darkening on the chroma too
    shade = np.maximum(grime * lines, 0.25)
    color = np.clip(ycbcr_to_rgb(color_y, cb_hi / shade, cr_hi / shade), 0, 1)

    exported = {}
    if args.normal_map or args.slope_map:
        # Height in pixels of the original image, so the settings don't depend on the scale.
        # Grime is paint, not shape, so it's left out.
        # The lines' darkness varies along them, but their depth shouldn't: most of a line
        # gets the full depth, and only its faint edges are shallower
        groove = ndimage.gaussian_filter(np.clip((1 - lines) / 0.12, 0, 1), sigma=0.35 * args.scale)
        h = -args.groove_depth * groove
        if args.relief_strength > 0:
            relief = shading_relief(clean, args.line_size, args.light_dir)
            h += args.relief_strength * resize(relief, out_width, out_height)
        if args.detail_strength > 0:
            low = ndimage.gaussian_filter(surface_hi, sigma=0.5 * args.scale)
            h += args.detail_strength * (surface_hi - low) / np.maximum(low, 0.1)
        # Slopes per pixel of the original image
        normals = height_to_normals(h * args.scale, args.normal_strength, not args.normal_green_down)
        if args.normal_map:
            exported["normal"] = to_image(normals, "RGB")
            exported["height"] = to_image16(h)
        if args.slope_map:
            # White where the normal points straight out of the surface, darker as it tilts
            # away. Based on the angle, so gentle slopes still show up.
            angle = np.arccos(np.clip(normals[..., 2] * 2 - 1, 0, 1))
            exported["slope"] = to_image(1 - angle / (np.pi / 2) * args.slope_contrast, "L")

    # Remove the shading painted into the texture from the color layer. It's estimated on the
    # clean surface (without the lines and the grime), whose sharp edges would otherwise
    # count as paint anyway.
    shading_hi = None
    if args.flatten_shading > 0:
        shading_hi = resize(estimate_shading(clean, args.flatten_threshold), out_width, out_height)
        color = np.clip(color * np.exp(-args.flatten_shading * shading_hi)[..., None], 0, 1)

    # Adjust the layers and combine them
    color = adjust_color(color, args.color_brightness, args.color_contrast)
    grime = adjust_multiply_layer(grime, args.grime_brightness, args.grime_contrast)
    grime = tint_multiply_layer(grime, args.grime_color)
    lines = adjust_multiply_layer(lines, args.lines_brightness, args.lines_contrast)
    if grime.ndim == 2:
        grime = grime[..., None]
    rgb_hi = np.clip(color * grime * lines[..., None], 0, 1)
    # Compared with a plain upscale of the original, pixel by pixel
    original_value = None
    has_lit = lit is not None and lit.any()
    if args.bloom_threshold > 0 or has_lit:
        original_hi = np.clip(np.stack([resize(rgb[..., c], out_width, out_height) for c in range(3)], axis=-1), 0, 1)
    if args.bloom_threshold > 0:
        original_value = original_hi.max(axis=-1)
        rgb_hi = keep_bloom_side(rgb_hi, original_value, args.bloom_threshold)
    if has_lit:
        # The lit pixels keep the original colors
        rgb_hi[lit] = original_hi[lit]

    mode = "RGB"
    if has_alpha:
        alpha_hi = np.clip(resize(alpha, out_width, out_height), 0, 1)
        rgb_hi = np.concatenate([rgb_hi, alpha_hi[..., None]], axis=-1)
        color = np.concatenate([color, alpha_hi[..., None]], axis=-1)
        mode = "RGBA"
    result = to_image(rgb_hi, mode)
    if args.indexed:
        mixed = lit is not None and lit.any() and not lit.all()
        result = to_indexed(result, 256 - ILLUM_RESERVED_COLORS if mixed else 256)
        if mixed:
            result = use_exact_lit_colors(result, rgb_hi, lit)
        if original_value is not None:
            result = fix_indexed_bloom(result, original_value, args.bloom_threshold)
        if mixed:
            result = separate_lit_colors(result, lit, args.bloom_threshold)

    if args.export_layers:
        exported["color"] = to_image(color, mode)
        # The color layer split in two: its brightness, and its hue and saturation at a
        # constant mid brightness. The Color blend mode puts them back together.
        rgb = color[..., :3]
        gray = rgb @ BLEND_LUM
        # Saturated colors can't all reach mid brightness without clipping (and losing
        # saturation), so they stay as close to it as they can
        lowest = gray - rgb.min(axis=-1)
        highest = gray + 1 - rgb.max(axis=-1)
        chroma = set_lum(rgb, np.clip(0.5, lowest, highest))
        if has_alpha:
            gray = np.stack([gray, alpha_hi], axis=-1)
            chroma = np.concatenate([chroma, alpha_hi[..., None]], axis=-1)
        exported["color_gray"] = to_image(gray, "LA" if has_alpha else "L")
        exported["color_chroma"] = to_image(chroma, mode)
        exported["grime"] = to_image(grime, "RGB") if grime.shape[-1] == 3 else to_image(grime[..., 0], "L")
        exported["lines"] = to_image(lines, "L")
        if shading_hi is not None:
            # The shading that was removed, as a multiply factor: 0.5 is no change, darker
            # is where the texture had painted shadows
            exported["shading"] = to_image(0.5 * np.exp(shading_hi), "L")
    debug_layers = {
        "brightness": y,
        "base": base,
        "lines": dark_lines,
        "lines_thin": dark_thin,
    }
    return result, exported, debug_layers


def is_texture_map(path):
    """True for the alpha and illumination maps that XwaOptEditor exports with the textures."""
    return os.path.splitext(path)[0].lower().endswith(MAP_SUFFIXES)


def resize_map(image, scale):
    """
    Plain Lanczos upscale, for alpha and illumination maps. They have no panel lines or
    grime, and they must stay the same size as their texture.
    """
    if image.mode == "P":
        image = image.convert("RGBA" if "transparency" in image.info else "RGB")
    elif image.mode not in ("L", "LA", "RGB", "RGBA"):
        image = image.convert("RGBA")
    return image.resize((image.width * scale, image.height * scale), Image.LANCZOS)


def make_palette(image, colors=256, iterations=15):
    """
    A palette of up to `colors` colors for an RGB image. It starts from Pillow's max coverage
    palette and refines it with k-means over the image's colors, each one weighted by the
    square root of its pixel count. Pillow's median cut gives the rare colors (like thin
    dark lines) too little weight and can tint them with a nearby common color; this keeps
    them close to their own color.
    """
    pixels = np.asarray(image, dtype=np.uint8).reshape(-1, 3)
    packed = (pixels[:, 0].astype(np.int32) << 16) | (pixels[:, 1].astype(np.int32) << 8) | pixels[:, 2]
    unique, counts = np.unique(packed, return_counts=True)
    points = np.stack([(unique >> 16) & 255, (unique >> 8) & 255, unique & 255], axis=1).astype(np.float32)
    if len(points) <= colors:
        return points
    weights = np.sqrt(counts).astype(np.float32)
    start = image.quantize(colors=colors, method=Image.Quantize.MAXCOVERAGE, dither=Image.Dither.NONE)
    centers = np.asarray(start.getpalette()[:colors * 3], dtype=np.float32).reshape(-1, 3)
    for _ in range(iterations):
        _, nearest = cKDTree(centers).query(points)
        sums = np.zeros_like(centers)
        np.add.at(sums, nearest, points * weights[:, None])
        totals = np.bincount(nearest, weights=weights, minlength=len(centers))
        used = totals > 0
        centers[used] = sums[used] / totals[used, None]
        # Unused palette entries go to the colors that are represented worst
        if not used.all():
            error = np.linalg.norm(points - centers[nearest], axis=1)
            centers[~used] = points[np.argsort(error)[-int((~used).sum()):]]
    return centers


# The engine makes the illuminated areas of a texture bloom when the brightest channel of
# their color (HSV value) reaches 0.75 (fully at 0.85). See XwaD3dPixelShader.hlsl,
# PixelShaderAnim.hlsl and PixelShaderAnimDAT.hlsl: smoothstep(0.75, 0.85, val).
DEFAULT_BLOOM_THRESHOLD = 0.75
# Distance kept below the threshold, so 8-bit rounding and the dithering of --indexed don't
# push a pixel over it
BLOOM_MARGIN = 3.0 / 255.0


def keep_bloom_side(rgb, original_value, threshold):
    """
    Keeps every pixel on the side of the bloom threshold it was on in the original texture,
    comparing the brightest channel of each (original_value is that of the original). Pixels
    that were below are capped a little under the threshold, so the processing doesn't make
    them glow in game. Pixels that were above don't get darker than they were, so lights glow
    as much as before. The whole color is scaled, so the hue doesn't change.
    """
    value = rgb.max(axis=-1)
    cap = threshold - BLOOM_MARGIN
    scale = np.ones_like(value)
    below = (original_value < threshold) & (value > cap)
    scale[below] = cap / value[below]
    # A little above the threshold too, so they don't round to just under it in 8 bits
    floor = np.maximum(original_value, threshold + BLOOM_MARGIN)
    above = (original_value >= threshold) & (value < floor) & (value > 1e-4)
    scale[above] = floor[above] / value[above]
    return np.clip(rgb * scale[..., None], 0, 1)


def fix_indexed_bloom(image, original_value, threshold):
    """
    After an image is indexed, its palette and dithering can move a pixel to the other side
    of the bloom threshold. Those pixels get the closest palette color on the side they were
    on in the original texture (with the same alpha, for images with transparency).
    """
    if image.mode != "P":
        return image
    rawmode = image.palette.mode if image.palette.mode in ("RGB", "RGBA") else "RGB"
    channels = len(rawmode)
    indices = np.asarray(image)
    palette = np.asarray(image.getpalette(rawmode), dtype=np.float32).reshape(-1, channels) / 255.0
    palette = palette[:max(int(indices.max()) + 1, 1)]
    palette_value = palette[:, :3].max(axis=1)
    current_value = palette_value[indices]
    wrong_side = (original_value < threshold) != (current_value < threshold)
    if channels == 4:
        # Fully transparent pixels don't show
        wrong_side &= palette[indices, 3] > 0
    if not wrong_side.any():
        return image
    indices = indices.copy()
    # The alpha weighs much more than the color, so the pixels keep their transparency
    weights = np.array([1.0, 1.0, 1.0, 100.0][:channels], dtype=np.float32)
    for below in (True, False):
        pixels = wrong_side & ((original_value < threshold) == below)
        allowed = np.nonzero((palette_value < threshold) == below)[0]
        if not pixels.any() or len(allowed) == 0:
            continue
        _, nearest = cKDTree(palette[allowed] * weights).query(palette[indices[pixels]] * weights)
        indices[pixels] = allowed[nearest]
    fixed = Image.fromarray(indices.astype(np.uint8), "P")
    fixed.putpalette(image.getpalette(rawmode), rawmode)
    fixed.info = dict(image.info)
    return fixed


def to_indexed(image, colors=256):
    """
    Converts an image to indexed color (a palette of up to `colors` colors), with
    Floyd-Steinberg dithering so smooth gradients don't band. Images with that many colors
    or less keep their exact colors. Opaque images use make_palette(); images with
    transparency use Pillow's fast octree, the only one of its methods that keeps the alpha.
    """
    if image.mode == "P":
        return image
    if image.mode in ("RGBA", "LA"):
        image = image.convert("RGBA")
        if image.getextrema()[3][0] < 255:
            return image.quantize(colors=colors, method=Image.Quantize.FASTOCTREE,
                                  dither=Image.Dither.FLOYDSTEINBERG)
    image = image.convert("RGB")
    # Few enough colors: each pixel gets its own color. (Pillow's quantize() isn't exact for
    # them: it can map a pixel to a color a few levels away, even when its color is there.)
    pixels = np.asarray(image, dtype=np.uint8).reshape(-1, 3)
    unique, inverse = np.unique(pixels, axis=0, return_inverse=True)
    if len(unique) <= colors:
        exact = Image.fromarray(inverse.reshape(image.height, image.width).astype(np.uint8), "P")
        exact.putpalette(unique.ravel().tolist())
        return exact
    palette_colors = np.clip(np.round(make_palette(image, colors)), 0, 255).astype(np.uint8)
    palette = Image.new("P", (1, 1))
    palette.putpalette(palette_colors.ravel().tolist())
    return image.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG)


# Palette entries left free in indexed textures that have lit and unlit pixels, for
# separate_lit_colors()
ILLUM_RESERVED_COLORS = 16


def rgb565_keys(color):
    """
    The RGB565 colors that an 8-bit color can become in an 8-bit OPT palette, rounded down
    and to the nearest.
    """
    r, g, b = (int(v) for v in color[:3])
    return {(r >> 3, g >> 2, b >> 3), ((r * 31 + 127) // 255, (g * 63 + 127) // 255, (b * 31 + 127) // 255)}


# Color offsets for separate_lit_colors(), closest first
_NUDGES = sorted(((dr, dg, db) for dr in range(-16, 17) for dg in range(-8, 9) for db in range(-16, 17)
                  if (dr, dg, db) != (0, 0, 0)), key=lambda d: d[0] ** 2 + d[1] ** 2 + d[2] ** 2)


def separate_lit_colors(image, lit, bloom_threshold=0.0):
    """
    In an indexed texture, gives the lit pixels (the ones the illumination map lights)
    colors that no unlit pixel has. The OPT library (XwaOptEditor, update_opt_textures.ps1)
    makes illumination a property of the colors of an indexed image, as they are in the OPT
    palette (RGB565): an unlit pixel with the same RGB565 color as a lit one is lit in game
    too. So a palette entry that both use is duplicated for the lit pixels, and the unlit
    colors that are the same as a lit one in RGB565 are moved to the closest color that
    isn't, without crossing the bloom threshold. The lit colors don't change. Without a
    free entry for a duplicate, those lit pixels get the closest lit color.
    """
    if image.mode != "P":
        return image
    rawmode = image.palette.mode if image.palette.mode in ("RGB", "RGBA") else "RGB"
    channels = len(rawmode)
    indices = np.asarray(image).copy()
    palette = np.asarray(image.getpalette(rawmode), dtype=np.int32).reshape(-1, channels)
    palette = [tuple(int(v) for v in c) for c in palette[:int(indices.max()) + 1]]
    unlit_entries = set(np.unique(indices[~lit]).tolist())
    lit_entries = set(np.unique(indices[lit]).tolist())

    # Entries that both use: the lit pixels get a copy of it
    leftover = []
    for entry in sorted(lit_entries & unlit_entries):
        if len(palette) < 256:
            palette.append(palette[entry])
            indices[lit & (indices == entry)] = len(palette) - 1
            lit_entries.add(len(palette) - 1)
        else:
            leftover.append(entry)
        lit_entries.discard(entry)
    if leftover:
        lit_only = sorted(lit_entries)
        if lit_only:
            tree = cKDTree(np.array([palette[i] for i in lit_only], dtype=np.float32))
            for entry in leftover:
                _, nearest = tree.query(np.array(palette[entry], dtype=np.float32))
                indices[lit & (indices == entry)] = lit_only[nearest]

    lit_keys = set()
    for i in lit_entries:
        lit_keys |= rgb565_keys(palette[i])

    def same_bloom_side(a, b):
        if bloom_threshold <= 0:
            return True
        return (max(a[:3]) < 255 * bloom_threshold) == (max(b[:3]) < 255 * bloom_threshold)

    # Unlit colors that are the same as a lit one in RGB565: moved
    for entry in unlit_entries:
        if not (rgb565_keys(palette[entry]) & lit_keys):
            continue
        color = palette[entry]
        for step in _NUDGES:
            c = tuple(min(max(color[k] + step[k], 0), 255) for k in range(3))
            if not (rgb565_keys(c) & lit_keys) and same_bloom_side(c, color):
                palette[entry] = c + tuple(color[3:])
                break
    fixed = Image.fromarray(indices.astype(np.uint8), "P")
    fixed.putpalette([v for c in palette for v in c], rawmode)
    fixed.info = dict(image.info)
    return fixed


def use_exact_lit_colors(image, rgb, lit):
    """
    Gives the lit pixels of an indexed texture their exact colors (from rgb, the image
    before indexing, 0..1), as palette entries of their own, when there are few of them
    (lights are usually painted with a few flat colors). Otherwise they keep the colors
    the palette and the dithering gave them.
    """
    if image.mode != "P":
        return image
    rawmode = image.palette.mode if image.palette.mode in ("RGB", "RGBA") else "RGB"
    channels = len(rawmode)
    indices = np.asarray(image).copy()
    palette = np.asarray(image.getpalette(rawmode), dtype=np.int32).reshape(-1, channels)
    palette = palette[:int(indices.max()) + 1]
    pixels = (np.clip(rgb[lit], 0, 1) * 255 + 0.5).astype(np.int32)
    if pixels.shape[1] < channels:
        pixels = np.concatenate([pixels, np.full((len(pixels), channels - pixels.shape[1]), 255, np.int32)], axis=1)
    pixels = pixels[:, :channels]
    colors, inverse = np.unique(pixels, axis=0, return_inverse=True)
    if len(palette) + len(colors) > 256:
        return image
    indices[lit] = len(palette) + inverse.ravel()
    fixed = Image.fromarray(indices.astype(np.uint8), "P")
    fixed.putpalette(np.concatenate([palette, colors]).ravel().tolist(), rawmode)
    fixed.info = dict(image.info)
    return fixed


def read_lit_mask(illum_path, args):
    """
    The pixels that an illumination map lights, at the output size: the ones that aren't
    black in the map this script writes for it.
    """
    with Image.open(illum_path) as illum:
        illum.load()
        illum = resize_map(illum, args.scale)
    if args.indexed:
        illum = to_indexed(illum)
    return np.asarray(illum.convert("RGB")).max(axis=-1) > 0


def save_layer(channel, path, scale_to_max=False):
    data = channel / max(channel.max(), 1e-6) if scale_to_max else channel
    Image.fromarray((np.clip(data, 0, 1) * 255 + 0.5).astype(np.uint8), "L").save(path)


def find_illum_map(path):
    """The NAME_illum image next to the texture NAME, or None."""
    folder, name = os.path.split(path)
    wanted = os.path.splitext(name)[0].lower() + "_illum"
    for other in sorted(os.listdir(folder or ".")):
        if os.path.splitext(other)[0].lower() == wanted:
            return os.path.join(folder, other)
    return None


def find_images(inputs, extensions, recursive):
    """Yields (input path, path relative to its input root) for every image to process."""
    for item in inputs:
        if os.path.isfile(item):
            yield item, os.path.basename(item)
        elif os.path.isdir(item):
            for root, dirs, files in os.walk(item):
                if not recursive:
                    dirs.clear()
                for name in sorted(files):
                    if os.path.splitext(name)[1].lower() in extensions:
                        path = os.path.join(root, name)
                        yield path, os.path.relpath(path, item)
        else:
            print("Not found: " + item, file=sys.stderr)


def process_batch(images, output_root, args, counts):
    """
    Processes the (path, relative path) images into output_root, keeping their relative
    paths, and updates counts. Returns {relative path: output path} for the images that
    have an output afterwards: the processed ones, and the ones skipped because their
    output already exists.
    """
    total = len(images)
    outputs = {}
    for index, (path, relative) in enumerate(images, start=1):
        # The lines are flushed right away, so they show up even when the output isn't a
        # console (for instance when it's piped or redirected to a file)
        progress = "[%*d/%d %3d%%]" % (len(str(total)), index, total, 100 * index // total)
        out_path = os.path.join(output_root, os.path.splitext(relative)[0] + ".png")
        if os.path.abspath(out_path) == os.path.abspath(path):
            print("%s Skipped (output would replace the input): %s" % (progress, path), flush=True)
            counts["skipped"] += 1
            continue
        if os.path.exists(out_path) and not args.overwrite:
            print("%s Skipped (exists, use --overwrite): %s" % (progress, out_path), flush=True)
            counts["skipped"] += 1
            outputs[relative] = out_path
            continue
        try:
            t = time.time()
            with Image.open(path) as image:
                image.load()
                if is_texture_map(path):
                    result, exported, layers = resize_map(image, args.scale), {}, None
                else:
                    lit = None
                    illum_path = find_illum_map(path)
                    if illum_path:
                        lit = read_lit_mask(illum_path, args)
                    result, exported, layers = process_image(image, args, lit)
            os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
            # process_image() indexes the textures itself; the alpha and illumination maps are
            # indexed here
            if args.indexed and is_texture_map(path):
                result = to_indexed(result)
            result.save(out_path)
            stem = os.path.splitext(out_path)[0]
            for name, layer in exported.items():
                layer_dir = os.path.dirname(out_path)
                if name in LAYER_SUBFOLDERS:
                    layer_dir = os.path.join(layer_dir, LAYER_SUBFOLDERS[name])
                    os.makedirs(layer_dir, exist_ok=True)
                layer.save(os.path.join(layer_dir, "%s_%s.png" % (os.path.basename(stem), name)))
            if args.save_layers and layers:
                save_layer(layers["brightness"], stem + "_layer_brightness.png")
                save_layer(layers["base"], stem + "_layer_base.png")
                save_layer(layers["lines"], stem + "_layer_lines.png", scale_to_max=True)
                save_layer(layers["lines_thin"], stem + "_layer_lines_thin.png", scale_to_max=True)
            print("%s %s -> %s (%dx%d, %.1f s)" % (progress, path, out_path, result.width, result.height,
                                                   time.time() - t), flush=True)
            counts["processed"] += 1
            outputs[relative] = out_path
        except Exception as e:
            print("%s FAILED: %s: %s" % (progress, path, e), file=sys.stderr, flush=True)
            counts["failed"] += 1
    return outputs


def prepare_unzip_dir(unzip_dir):
    """
    Creates the temporary folder the ZIP files are extracted to. A folder left behind by an
    earlier run is emptied, but a folder with that name that this script didn't make is
    left alone, so nothing of the user's is deleted.
    """
    marker = os.path.join(unzip_dir, UNZIP_MARKER)
    if os.path.isdir(unzip_dir):
        if os.listdir(unzip_dir) and not os.path.exists(marker):
            raise RuntimeError("%s already exists and wasn't made by this script. Move it or use "
                               "another output directory" % unzip_dir)
        shutil.rmtree(unzip_dir)
    os.makedirs(unzip_dir)
    open(marker, "w").close()


def extract_zip(zip_file, unzip_dir):
    """Extracts the files of the ZIP. Entries that would land outside unzip_dir are skipped."""
    root = os.path.abspath(unzip_dir)
    for info in zip_file.infolist():
        if info.is_dir():
            continue
        target = os.path.abspath(os.path.join(root, info.filename))
        if not target.startswith(root + os.sep):
            print("Skipped unsafe ZIP entry: " + info.filename, file=sys.stderr)
            continue
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with zip_file.open(info) as src, open(target, "wb") as dst:
            shutil.copyfileobj(src, dst)


def process_zip(zip_path, extensions, args, counts):
    """
    Processes the images in a ZIP file and writes a ZIP with the same name in the output
    directory. The images are extracted to OUTPUT/unzipped (deleted afterwards), and
    processed into OUTPUT/ZIPNAME, where the normal maps, height maps and other extra
    outputs also stay. The new ZIP only has the processed images, at the same paths as in
    the original ZIP (as PNG), and no folder entries: XWA's ZIP reader counts every entry
    in a group folder as an image.
    """
    zip_name = os.path.basename(zip_path)
    out_zip = os.path.join(args.output, zip_name)
    if os.path.normcase(os.path.abspath(out_zip)) == os.path.normcase(os.path.abspath(zip_path)):
        print("FAILED: %s: the new ZIP would replace it, use another output directory" % zip_path,
              file=sys.stderr, flush=True)
        counts["failed"] += 1
        return
    if os.path.exists(out_zip) and not args.overwrite:
        print("Skipped (exists, use --overwrite): " + out_zip, flush=True)
        counts["skipped"] += 1
        return

    unzip_dir = os.path.join(args.output, UNZIP_DIR)
    try:
        prepare_unzip_dir(unzip_dir)
        with zipfile.ZipFile(zip_path) as zip_file:
            extract_zip(zip_file, unzip_dir)
            other_files = sum(1 for info in zip_file.infolist() if not info.is_dir() and
                              os.path.splitext(info.filename)[1].lower() not in extensions)
        images = list(find_images([unzip_dir], extensions, recursive=True))
        print("Found %d image%s in %s" % (len(images), "" if len(images) == 1 else "s", zip_path), flush=True)
        if other_files:
            print("Left out of the new ZIP: %d file%s that %s images" % (
                other_files, "" if other_files == 1 else "s", "isn't" if other_files == 1 else "aren't"),
                flush=True)

        if not images:
            print("Not creating %s: there are no images in the ZIP" % out_zip, flush=True)
            return

        failed_before = counts["failed"]
        outputs = process_batch(images, os.path.join(args.output, os.path.splitext(zip_name)[0]), args, counts)
        if counts["failed"] > failed_before:
            print("Not creating %s: some images failed" % out_zip, file=sys.stderr, flush=True)
            return

        # Write to a temporary file first, so a failure doesn't leave a broken ZIP behind
        names = set()
        temp_zip = out_zip + ".tmp"
        with zipfile.ZipFile(temp_zip, "w", zipfile.ZIP_DEFLATED) as new_zip:
            for relative in sorted(outputs):
                name = os.path.splitext(relative)[0].replace(os.sep, "/") + ".png"
                if name.lower() in names:
                    print("Warning: %s is in the ZIP twice (with different extensions), only the "
                          "first one is kept" % name, file=sys.stderr, flush=True)
                    continue
                names.add(name.lower())
                new_zip.write(outputs[relative], name)
        os.replace(temp_zip, out_zip)
        print("Created %s (%d images)" % (out_zip, len(names)), flush=True)
    except Exception as e:
        print("FAILED: %s: %s" % (zip_path, e), file=sys.stderr, flush=True)
        counts["failed"] += 1
    finally:
        if os.path.exists(os.path.join(unzip_dir, UNZIP_MARKER)):
            shutil.rmtree(unzip_dir, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(
        description="Upscale textures while making their panel lines thinner.")
    parser.add_argument("inputs", nargs="+",
                        help="Image files, directories or ZIP files. The images in a ZIP file are processed "
                             "into OUTPUT/ZIPNAME, and a new ZIP with the same name and only the processed "
                             "images is saved in OUTPUT")
    parser.add_argument("-o", "--output", required=True,
                        help="Output directory. The folder structure of the inputs is kept.")
    parser.add_argument("--scale", type=int, default=4, help="Scale factor (default: 4)")
    parser.add_argument("--line-width", type=float, default=0.5,
                        help="Width of the lines relative to a plain upscale. 1 keeps them as they are, "
                             "0.5 makes them half as wide (default: 0.5)")
    parser.add_argument("--line-size", type=int, default=7,
                        help="Widest line to thin, in pixels of the original image (default: 7)")
    parser.add_argument("--line-threshold", type=float, default=0.03,
                        help="How much darker than its surroundings (0..1) a pixel must be to count as a line. "
                             "Lower values also thin fainter details (default: 0.03)")
    parser.add_argument("--shape-filter", type=float, default=0.7,
                        help="0..1: how strictly only line-shaped features are thinned. 0 also thins "
                             "spots and scratches (default: 0.7)")
    parser.add_argument("--min-line-length", type=int, default=30,
                        help="Only thin lines that are part of connected structures at least this big, in pixels "
                             "of the original image. Protects small details like handles and rivets. "
                             "0 thins everything (default: 30)")
    parser.add_argument("--raised-threshold", type=float, default=0.2,
                        help="Dark lines next to narrow highlights at least this bright (0..1 above their "
                             "surroundings) are treated as shadows of raised details (bars, pins, handles) and "
                             "aren't thinned. Lower it if details get thinned, raise it if grooves next to "
                             "bevel highlights are skipped. 0 disables it (default: %(default)s)")
    parser.add_argument("--thin-highlights", action="store_true",
                        help="Also thin narrow bright lines, like bevel highlights")
    parser.add_argument("--no-guided-chroma", action="store_true",
                        help="Upscale the color without snapping it to the brightness edges")
    parser.add_argument("--grime-size", type=int, default=15,
                        help="Largest dark detail that counts as grime instead of part of the surface, in "
                             "pixels of the original image. 0 leaves all the grime in the color layer "
                             "(default: %(default)s)")
    layers = parser.add_argument_group(
        "layer adjustments",
        "Brightness is added (-1..1, 0 = unchanged); contrast is a multiplier (1 = unchanged). For the "
        "color layer they work like in an image editor. For grime and lines, contrast scales how dark "
        "they are (0.5 = half as dark, 0 removes them) and brightness lightens (positive) or darkens "
        "(negative) their dark parts by a fixed amount.")
    for name in ("color", "grime", "lines"):
        layers.add_argument("--%s-brightness" % name, type=float, default=0.0, metavar="B",
                            help="Brightness of the %s layer (default: 0)" % name)
        layers.add_argument("--%s-contrast" % name, type=float, default=1.0, metavar="C",
                            help="Contrast of the %s layer (default: 1)" % name)
    layers.add_argument("--bloom-threshold", type=float, default=DEFAULT_BLOOM_THRESHOLD, metavar="T",
                        help="The engine makes the illuminated areas of a texture bloom when the brightest of "
                             "their R, G and B reaches this value (0..1). Every pixel of the result is kept on the "
                             "side of it that it was on in the original texture, so brightening a texture doesn't "
                             "make new areas glow, and lights still glow. 0 turns it off (default: %(default)s)")
    layers.add_argument("--flatten-shading", type=float, default=0.0, metavar="S",
                        help="Removes the shading painted into the texture (baked ambient occlusion, light "
                             "gradients) from the color layer, so the engine's lighting and ambient occlusion "
                             "don't darken the same places twice. 1 removes all of it, 0.5 half, 0 nothing. "
                             "Markings and panels keep their own brightness, and the fine detail is kept "
                             "(default: %(default)s)")
    layers.add_argument("--flatten-threshold", type=float, default=0.02, metavar="T",
                        help="For --flatten-shading: how sharp a change in brightness must be (relative change "
                             "per pixel of the original image) to count as paint instead of shading. Higher "
                             "values also flatten softer paint edges (default: %(default)s)")
    layers.add_argument("--grime-color", default="1,1,1", metavar="R,G,B",
                        help="Color of the dirt, 0..1 per channel: the grime darkens toward it instead of "
                             "toward gray, with the same brightness. For instance 0.6,0.45,0.3 for brown. "
                             "The further it is from gray, the stronger the tint. Gray or white keeps the "
                             "grime gray (default: %(default)s)")
    parser.add_argument("--export-layers", action="store_true",
                        help="Also save the color, grime and lines layers (NAME_color.png, NAME_grime.png, "
                             "NAME_lines.png), after the adjustments, so they can be combined by hand: "
                             "color x grime x lines, with the Multiply blend mode. The color layer is also "
                             "saved split in two: NAME_color_gray.png (brightness) and NAME_color_chroma.png "
                             "(hue and saturation). Chroma on top of gray with the Color blend mode makes the "
                             "color layer")
    normals = parser.add_argument_group(
        "normal map",
        "The height is built from the lines layer: the panel lines become grooves. The painted lighting on "
        "raised details (relief) and the fine surface texture (detail) can be added too. Heights are in "
        "pixels of the original image.")
    normals.add_argument("--normal-map", action="store_true",
                         help="Also save a tangent-space normal map (NormalMaps/NAME_normal.png) and the height "
                              "map it's made from (HeightMaps/NAME_height.png, 16-bit). The subfolders are made "
                              "next to the upscaled image")
    normals.add_argument("--slope-map", action="store_true",
                         help="Also save a grayscale slope map (NAME_slope.png): white where the surface is flat, "
                              "darker the more it tilts, black at 90 degrees. Uses the same settings as the "
                              "normal map")
    normals.add_argument("--slope-contrast", type=float, default=1.0,
                         help="Multiplies how dark the slope map gets for a given tilt (default: %(default)s)")
    normals.add_argument("--normal-strength", type=float, default=1.0,
                         help="Multiplies all the slopes in the normal map (default: %(default)s)")
    normals.add_argument("--groove-depth", type=float, default=0.5,
                         help="Depth of the panel lines (default: %(default)s)")
    normals.add_argument("--relief-strength", type=float, default=0.0,
                         help="Also add the shapes recovered from the painted lighting, at this height. 1 is "
                              "a good start. 0 leaves them out (default: %(default)s)")
    normals.add_argument("--detail-strength", type=float, default=0.0,
                         help="Also add the fine surface texture, at this height. 0.25 is a good start. 0 "
                              "leaves it out (default: %(default)s)")
    normals.add_argument("--light-dir", default="-1,-1",
                         help="For --relief-strength: direction of the light painted on the texture, as x,y "
                              "in image coordinates (x right, y down). -1,-1 is light from the top-left "
                              "(default: %(default)s)")
    normals.add_argument("--normal-green-down", action="store_true",
                         help="Store the down-facing component in green (DirectX style). By default green "
                              "stores the up-facing component (OpenGL style), which is what XWA uses")
    parser.add_argument("--recursive", action="store_true", help="Also process subdirectories")
    parser.add_argument("--extensions", default=DEFAULT_EXTENSIONS,
                        help="Comma-separated extensions to process in directories (default: %s)" % DEFAULT_EXTENSIONS)
    parser.add_argument("--indexed", action="store_true",
                        help="Save the upscaled images (and their _alpha and _illum maps) as indexed PNGs, with a "
                             "palette of up to 256 colors and dithering. They're much smaller, and they're also "
                             "what goes into the new ZIP of a ZIP input. Images with 256 colors or less keep "
                             "their exact colors. Normal maps, height maps and layers keep their full colors")
    parser.add_argument("--overwrite", action="store_true", help="Replace existing output files")
    parser.add_argument("--save-layers", action="store_true",
                        help="Also save the intermediate layers of the line extraction, to tune the parameters")
    args = parser.parse_args()

    if args.scale < 1:
        parser.error("--scale must be 1 or more")
    if not 0 < args.line_width <= 1:
        parser.error("--line-width must be between 0 and 1")
    for name in ("color", "grime", "lines"):
        if getattr(args, name + "_contrast") < 0:
            parser.error("--%s-contrast can't be negative" % name)
    try:
        args.grime_color = [float(v) for v in args.grime_color.split(",")]
        if len(args.grime_color) != 3 or not all(0 <= v <= 1 for v in args.grime_color):
            raise ValueError
    except ValueError:
        parser.error("--grime-color must be three numbers between 0 and 1, like 0.6,0.45,0.3")
    try:
        light = [float(v) for v in args.light_dir.split(",")]
        if len(light) != 2 or np.hypot(light[0], light[1]) == 0:
            raise ValueError
        length = np.hypot(light[0], light[1])
        args.light_dir = (light[0] / length, light[1] / length)
    except ValueError:
        parser.error("--light-dir must be two numbers, like -1,-1")
    extensions = {e.strip().lower() if e.strip().startswith(".") else "." + e.strip().lower()
                  for e in args.extensions.split(",") if e.strip()}

    os.makedirs(args.output, exist_ok=True)
    counts = {"processed": 0, "skipped": 0, "failed": 0}
    start = time.time()

    zip_inputs = [item for item in args.inputs if os.path.isfile(item) and item.lower().endswith(".zip")]
    other_inputs = [item for item in args.inputs if item not in zip_inputs]
    if other_inputs:
        # Find all the images first, so the progress can be shown as a percentage
        images = list(find_images(other_inputs, extensions, args.recursive))
        print("Found %d image%s" % (len(images), "" if len(images) == 1 else "s"), flush=True)
        process_batch(images, args.output, args, counts)
    for zip_path in zip_inputs:
        process_zip(zip_path, extensions, args, counts)

    print("Done: %d processed, %d skipped, %d failed in %.1f s" % (
        counts["processed"], counts["skipped"], counts["failed"], time.time() - start))
    return 1 if counts["failed"] else 0


if __name__ == "__main__":
    sys.exit(main())
