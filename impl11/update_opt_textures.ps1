<#
.SYNOPSIS
Replaces the textures of an OPT with the images in a directory.

.DESCRIPTION
For every texture in the OPT, looks for an image with the same name in the textures
directory (for instance HullTop.png for the texture HullTop) and replaces the texture
with it. The optional NAME_alpha and NAME_illum images become the texture's alpha and
illumination maps, which is how XwaOptEditor exports them. Textures without an image are
left as they are.

The OPT is read and written with JeremyAnsel.Xwa.Opt.dll, the library XwaOptEditor uses,
so the result is the same as replacing the textures one by one in the editor:
- Mipmaps are generated when the original texture had them.
- Indexed images (like the ones XwaOptEditor exports or upscale_textures.py --indexed
  makes) stay 8-bit with their own palette, rounded to RGB565 like every 8-bit OPT
  texture. Their mipmaps use the same palette. Indexed images with alpha or illumination
  can't stay 8-bit: they're stored as 32-bit, and there's a warning.
- 32-bit images that only use 256 colors or less (in RGB565) are stored as 8-bit, without
  loss, when the original texture was 8-bit. -Convert8 stores all of them as 8-bit, with a
  palette made by the OPT library.
- If the original texture has alpha or illumination and there's no image for it (a
  NAME_alpha or NAME_illum image, or for alpha, an alpha channel in the image), the
  texture is skipped, so they aren't lost.

Without -Output, the OPT is replaced, after saving a copy of it as NAME.OPT.bak.

.PARAMETER Opt
The OPT file.

.PARAMETER TexturesDir
The directory with the images.

.PARAMETER Output
Where to save the new OPT. By default, the OPT is replaced (after a backup).

.PARAMETER OptEditorDir
The XwaOptEditor directory that has JeremyAnsel.Xwa.Opt.dll and JeremyAnsel.ColorQuant.dll.

.PARAMETER Extensions
The image extensions to look for, in order of preference.

.PARAMETER Convert8
Stores every replaced texture as 8-bit (256 colors). Much smaller, but gradients can band.

.PARAMETER DryRun
Shows what would be replaced, without saving anything.

.EXAMPLE
.\update_opt_textures.ps1 "C:\XWA\FlightModels\XWingExterior.OPT" "C:\XWA\FlightModels\XWingExterior_x4"

.EXAMPLE
.\update_opt_textures.ps1 XWingExterior.OPT .\XWingExterior_x4 -Output XWingExterior_new.OPT -DryRun
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)] [string] $Opt,
    [Parameter(Mandatory = $true, Position = 1)] [string] $TexturesDir,
    [string] $Output,
    [string] $OptEditorDir = "C:\Zip\XWA-DX11\ZIP\XwaOptEditor-1.0.103",
    [string[]] $Extensions = @(".png", ".bmp", ".jpg", ".jpeg"),
    [switch] $Convert8,
    [switch] $DryRun
)

$ErrorActionPreference = "Stop"

function Find-Image([string] $name) {
    foreach ($ext in $Extensions) {
        $path = Join-Path $TexturesDir ($name + $ext)
        if (Test-Path -LiteralPath $path) { return $path }
    }
    return $null
}

function Test-PowerOfTwo([int] $n) { return $n -gt 0 -and ($n -band ($n - 1)) -eq 0 }

function Format-Size([long] $bytes) { return "{0:N1} MB" -f ($bytes / 1MB) }

$Opt = (Resolve-Path -LiteralPath $Opt).Path
$TexturesDir = (Resolve-Path -LiteralPath $TexturesDir).Path
foreach ($dll in @("JeremyAnsel.ColorQuant.dll", "JeremyAnsel.Xwa.Opt.dll")) {
    $path = Join-Path $OptEditorDir $dll
    if (-not (Test-Path -LiteralPath $path)) {
        throw "$dll not found in $OptEditorDir. Use -OptEditorDir to point to XwaOptEditor."
    }
    Add-Type -Path $path
}

# Mipmaps for 8-bit textures that keep their palette. The OPT library can only make
# mipmaps in 32-bit (and then picks a new palette when converting back to 8-bit), so
# indexed images would lose their palette. Each mip level of an 8-bit texture is just
# more palette indices after the base level, so they're made here: each level is the
# average of 2x2 pixels of the previous one, mapped to the nearest palette color.
Add-Type -Language CSharp -TypeDefinition @"
using System;
using System.Collections.Generic;

public static class OptMipmaps8
{
    // The colors of an 8-bit OPT texture: RGB565 at full brightness (the palette has 16
    // brightness levels, this is the one XwaOptEditor uses), expanded like the OPT library.
    static void GetColors(byte[] palette, int[] r, int[] g, int[] b)
    {
        for (int i = 0; i < 256; i++)
        {
            int c = BitConverter.ToUInt16(palette, 4096 + i * 2);
            r[i] = (((c & 0xF800) >> 11) * 510 + 31) / 62;
            g[i] = (((c & 0x07E0) >> 5) * 510 + 63) / 126;
            b[i] = ((c & 0x001F) * 510 + 31) / 62;
        }
    }

    public static byte[] Generate(byte[] imageData, byte[] palette, int width, int height)
    {
        int[] pr = new int[256], pg = new int[256], pb = new int[256];
        GetColors(palette, pr, pg, pb);
        int count = width * height;
        float[] r = new float[count], g = new float[count], b = new float[count];
        List<byte> levels = new List<byte>(count * 4 / 3 + 16);
        for (int i = 0; i < count; i++)
        {
            int k = imageData[i];
            r[i] = pr[k]; g[i] = pg[k]; b[i] = pb[k];
            levels.Add(imageData[i]);
        }

        Dictionary<int, byte> nearest = new Dictionary<int, byte>();
        int w = width, h = height;
        while (w > 1 || h > 1)
        {
            int nw = w <= 1 ? 1 : w / 2, nh = h <= 1 ? 1 : h / 2;
            float[] r2 = new float[nw * nh], g2 = new float[nw * nh], b2 = new float[nw * nh];
            for (int y = 0; y < nh; y++)
            {
                for (int x = 0; x < nw; x++)
                {
                    float sr = 0, sg = 0, sb = 0;
                    int n = 0;
                    for (int dy = 0; dy < 2; dy++)
                    {
                        int sy = Math.Min(y * 2 + dy, h - 1);
                        for (int dx = 0; dx < 2; dx++)
                        {
                            int sx = Math.Min(x * 2 + dx, w - 1);
                            int s = sy * w + sx;
                            sr += r[s]; sg += g[s]; sb += b[s]; n++;
                        }
                    }
                    int d = y * nw + x;
                    r2[d] = sr / n; g2[d] = sg / n; b2[d] = sb / n;

                    int ir = (int)(r2[d] + 0.5f), ig = (int)(g2[d] + 0.5f), ib = (int)(b2[d] + 0.5f);
                    int key = (ir << 16) | (ig << 8) | ib;
                    byte index;
                    if (!nearest.TryGetValue(key, out index))
                    {
                        int best = int.MaxValue;
                        for (int k = 0; k < 256; k++)
                        {
                            int er = ir - pr[k], eg = ig - pg[k], eb = ib - pb[k];
                            int e = er * er + eg * eg + eb * eb;
                            if (e < best) { best = e; index = (byte)k; }
                        }
                        nearest[key] = index;
                    }
                    levels.Add(index);
                }
            }
            r = r2; g = g2; b = b2;
            w = nw; h = nh;
        }
        return levels.ToArray();
    }
}
"@

Write-Host "Reading $Opt"
$optFile = [JeremyAnsel.Xwa.Opt.OptFile]::FromFile($Opt)

$replaced = 0; $skipped = 0; $missing = 0; $failed = 0
# Indexed images that can't stay 8-bit (see the warning at the end)
$repalettized = @()
$used = @{}
foreach ($name in @($optFile.Textures.Keys | Sort-Object)) {
    $old = $optFile.Textures[$name]
    $colorFile = Find-Image $name
    if ($null -eq $colorFile) {
        $missing++
        continue
    }
    $alphaFile = Find-Image ($name + "_alpha")
    $illumFile = Find-Image ($name + "_illum")
    $used[[IO.Path]::GetFileName($colorFile).ToLowerInvariant()] = $true
    if ($alphaFile) { $used[[IO.Path]::GetFileName($alphaFile).ToLowerInvariant()] = $true }
    if ($illumFile) { $used[[IO.Path]::GetFileName($illumFile).ToLowerInvariant()] = $true }

    try {
        $new = [JeremyAnsel.Xwa.Opt.Texture]::FromFile($colorFile, $alphaFile, $illumFile)
        $new.Name = $name

        if ($old.HasAlpha -and -not $new.HasAlpha) {
            Write-Warning "$name skipped: the texture has alpha, but there's no ${name}_alpha image and the image has no alpha channel"
            $skipped++
            continue
        }
        if ($old.IsIlluminated -and -not $new.IsIlluminated) {
            Write-Warning "$name skipped: the texture is illuminated, but there's no ${name}_illum image"
            $skipped++
            continue
        }
        if ($new.Width -lt $old.Width -or $new.Height -lt $old.Height) {
            Write-Warning "$name gets smaller: the texture is $($old.Width)x$($old.Height), the image is $($new.Width)x$($new.Height)"
        }
        if (-not (Test-PowerOfTwo $new.Width) -or -not (Test-PowerOfTwo $new.Height)) {
            Write-Warning "$name is $($new.Width)x$($new.Height), which isn't a power of two"
        }

        if ($new.BitsPerPixel -eq 8 -and -not $new.HasAlpha -and -not $new.IsIlluminated) {
            # An indexed image stays 8-bit with its own palette (rounded to RGB565, like
            # every 8-bit OPT texture), and its mipmaps use the same palette
            if ($old.MipmapsCount -gt 1) {
                $new.ImageData = [OptMipmaps8]::Generate($new.ImageData, $new.Palette, $new.Width, $new.Height)
            }
        }
        else {
            # Like XwaOptEditor: the mipmaps are made in 32-bit, and then the texture is
            # converted to 8-bit if needed. Whether it fits in 256 colors is checked first,
            # because the mipmaps add blended colors. (Convert8To32 also makes mipmaps.)
            if ($new.BitsPerPixel -eq 8) {
                # Indexed, with alpha or illumination: the library can't keep it 8-bit
                $repalettized += $name
                $new.Convert8To32()
                $fitsIn8 = $false
            }
            else {
                $fitsIn8 = $new.CanBeConvertedWithoutLoss()
            }
            if ($old.MipmapsCount -gt 1) {
                $new.GenerateMipmaps()
            }
            elseif ($new.MipmapsCount -gt 1) {
                $new.RemoveMipmaps()
            }
            if ($Convert8 -or ($old.BitsPerPixel -eq 8 -and $fitsIn8)) {
                $new.Convert32To8()
            }
        }

        $flags = @()
        if ($new.HasAlpha) { $flags += "alpha" }
        if ($new.IsIlluminated) { $flags += "illum" }
        Write-Host ("  {0,-24} {1,5}x{2,-5} {3,2}-bit -> {4,5}x{5,-5} {6,2}-bit  {7}" -f $name,
            $old.Width, $old.Height, $old.BitsPerPixel, $new.Width, $new.Height, $new.BitsPerPixel, ($flags -join ", "))

        $optFile.Textures[$name] = $new
        $replaced++
    }
    catch {
        Write-Warning "$name failed: $($_.Exception.Message)"
        $failed++
    }
}

# Images that don't belong to any texture, probably misnamed
$unused = @(Get-ChildItem -LiteralPath $TexturesDir -File |
    Where-Object { $Extensions -contains $_.Extension.ToLowerInvariant() -and -not $used.ContainsKey($_.Name.ToLowerInvariant()) } |
    ForEach-Object { $_.Name })
if ($unused.Count -gt 0) {
    Write-Host "Images that don't match any texture: $($unused -join ', ')"
}

if ($repalettized.Count -gt 0) {
    $what = if ($Convert8) { "-Convert8 makes a new palette for them, so their colors are reduced twice" }
            else { "they're stored as 32-bit, so they don't make the OPT smaller" }
    Write-Warning ("{0} indexed image(s) with alpha or illumination can't keep their palette in the OPT: {1}. For these textures, use images that aren't indexed. Textures: {2}" -f
        $repalettized.Count, $what, ($repalettized -join ", "))
}

Write-Host ("Replaced {0}, skipped {1}, failed {2}, without an image {3} (of {4} textures)" -f
    $replaced, $skipped, $failed, $missing, $optFile.Textures.Count)

if ($DryRun) {
    Write-Host "Dry run: nothing was saved"
    exit $(if ($failed -gt 0) { 1 } else { 0 })
}
if ($replaced -eq 0) {
    Write-Host "Nothing to save"
    exit $(if ($failed -gt 0) { 1 } else { 0 })
}

$oldSize = (Get-Item -LiteralPath $Opt).Length
if ($Output) {
    $target = [IO.Path]::GetFullPath($Output)
}
else {
    $target = $Opt
    $backup = $Opt + ".bak"
    if (Test-Path -LiteralPath $backup) {
        $backup = $Opt + "." + (Get-Date -Format "yyyyMMdd-HHmmss") + ".bak"
    }
    Copy-Item -LiteralPath $Opt -Destination $backup
    Write-Host "Backup: $backup"
}

# Save to a temporary file first, so a failure doesn't leave a broken OPT behind
$temp = $target + ".tmp"
$optFile.Save($temp)
Move-Item -LiteralPath $temp -Destination $target -Force
$newSize = (Get-Item -LiteralPath $target).Length
Write-Host "Saved $target ($(Format-Size $oldSize) -> $(Format-Size $newSize))"
if ($newSize -gt 300MB) {
    Write-Warning "The OPT is big. XWA is a 32-bit program, and large OPTs can make missions fail to load. Consider -Convert8 or a smaller scale for some textures."
}
exit $(if ($failed -gt 0) { 1 } else { 0 })
