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
- 32-bit images that only use 256 colors or less are stored as 8-bit textures, without
  loss, when the original texture was 8-bit. -Convert8 stores all of them as 8-bit.
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

Write-Host "Reading $Opt"
$optFile = [JeremyAnsel.Xwa.Opt.OptFile]::FromFile($Opt)

$replaced = 0; $skipped = 0; $missing = 0; $failed = 0
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

        # Like XwaOptEditor: the mipmaps are generated in 32-bit, and then the texture is
        # converted to 8-bit if needed. Whether it fits in 256 colors is checked before the
        # mipmaps, which add blended colors.
        if ($new.BitsPerPixel -eq 8) {
            $new.Convert8To32()
        }
        $fitsIn8 = $new.CanBeConvertedWithoutLoss()
        if ($old.MipmapsCount -gt 1) {
            $new.GenerateMipmaps()
        }
        if ($Convert8 -or ($old.BitsPerPixel -eq 8 -and $fitsIn8)) {
            $new.Convert32To8()
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
