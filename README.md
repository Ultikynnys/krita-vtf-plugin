# VTF import and export for Krita

Native Valve Texture Format (`.vtf`) support for Krita on Windows.

The plugin integrates with Krita's normal **File > Open** and **File > Save As** workflows. Saving a VTF opens a dedicated export-options dialog for selecting the VTF version, image encoding, mipmaps, thumbnail, bump-map scale, and texture flags.

## Features

- Opens and saves `.vtf` files through Krita's standard file dialogs.
- Uses a standalone Qt 5 `QImageIOPlugin` without linking against Krita's private C++ ABI.
- Provides a persistent VTF export-options dialog.
- Supports compressed and uncompressed image formats.
- Generates complete mipmap chains and low-resolution thumbnails.
- Validates incompatible settings instead of silently changing them.
- Builds and tests the Windows plugin in GitHub Actions.

## Import support

The importer supports VTF 7.x files and decodes:

- RGBA8888
- ABGR8888
- RGB888
- BGR888
- RGB565
- I8
- IA88
- A8
- ARGB8888
- BGRA8888
- BGRX8888
- DXT1
- DXT1 one-bit alpha
- DXT3
- DXT5

The current importer reads the first frame, first face, and full-resolution mip level into a Krita image.

## Export options

### VTF version

The export dialog offers:

- VTF 7.0
- VTF 7.1
- VTF 7.2
- VTF 7.3
- VTF 7.4
- VTF 7.5

### Image format

The writer supports:

| Format | Compression | Alpha |
| --- | --- | --- |
| DXT1 | Block compressed | No |
| DXT1 one-bit alpha | Block compressed | 1 bit |
| DXT3 | Block compressed | Explicit 4-bit alpha |
| DXT5 | Block compressed | Interpolated alpha |
| RGBA8888 | Uncompressed | 8 bit |
| BGRA8888 | Uncompressed | 8 bit |
| ABGR8888 | Uncompressed | 8 bit |
| ARGB8888 | Uncompressed | 8 bit |
| RGB888 | Uncompressed | No |
| BGR888 | Uncompressed | No |
| BGRX8888 | Uncompressed | No |
| RGB565 | Uncompressed | No |
| I8 | Uncompressed intensity | No |
| IA88 | Uncompressed intensity | 8 bit |
| A8 | Uncompressed alpha | 8 bit |

### Mipmaps and thumbnail

- Generate a complete mipmap chain down to 1 × 1.
- Generate an optional DXT1 low-resolution thumbnail.
- Select a thumbnail size from 1 to 64 pixels.
- Set the VTF bump-map scale.

Mipmapped textures must have power-of-two dimensions. The export dialog reports invalid dimensions and prevents the export rather than silently disabling mipmaps.

### Texture flags

Sampling and wrapping flags:

- Point sampling
- Trilinear filtering
- Clamp S
- Clamp T
- Clamp U
- Anisotropic filtering
- Border

Texture usage flags:

- Hint DXT5
- PWL corrected
- Normal map
- No LOD
- All mips
- Procedural
- Environment map
- Render target
- Depth render target
- No debug override
- Single copy
- Pre-sRGB
- No depth buffer
- Vertex texture
- SSBump

The writer synchronizes `NOMIP` with the mipmap setting. It also derives the one-bit or eight-bit alpha flag from the selected image format, preventing contradictory headers.

Environment maps must be square and use power-of-two dimensions. Invalid settings are shown in the dialog and cannot be accepted.

## Recommended export profiles

| Use | Format | Mipmaps | Suggested flags |
| --- | --- | --- | --- |
| Opaque color texture | DXT1 | Enabled | Trilinear |
| Color texture with smooth alpha | DXT5 | Enabled | Trilinear |
| Hard-edge transparency | DXT1 one-bit alpha | Enabled | Trilinear |
| Lossless texture | RGBA8888 | As needed | Trilinear |
| Normal map | DXT5 or RGBA8888 | Enabled | Normal map, Trilinear |
| User interface texture | DXT5 or RGBA8888 | Often disabled | Clamp S, Clamp T |

These are starting points, not engine requirements. Select settings appropriate for the target Source-engine toolchain and material.

## Installation

### Requirements

- Windows x64
- Krita installed at `C:\Program Files\Krita (x64)` by default
- Administrator access for modifying the Krita installation

### Install

The compiled Windows plugin is included in the repository. You do not need to build it or download a GitHub Actions artifact.

1. Download the repository as a ZIP from GitHub or clone it with Git.
2. Extract the ZIP if necessary.
3. Fully close Krita.
4. Open PowerShell as Administrator in the repository directory.
5. Run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install.ps1
```

For a non-default Krita installation, provide its root directory:

```powershell
.\install.ps1 -KritaRoot "D:\Applications\Krita"
```

Restart Krita after installation.

The repository includes the complete installer payload:

```text
bin/imageformats/kimg_vtf.dll
share/mime/packages/vtf.xml
install.ps1
```

### What the installer changes

The installer:

- Copies `kimg_vtf.dll` to Krita's `bin\imageformats` directory.
- Copies the VTF MIME description to `%LOCALAPPDATA%\mime\packages\vtf.xml`.
- Registers `.vtf` as `image/vnd.valve.source.texture` for the current Windows user.
- Creates a timestamped backup inside the Krita installation.
- Applies a byte-length-preserving MIME metadata patch to Krita's existing QImageIO import and export bridge DLLs.

The metadata patch is necessary because Krita's generic QImageIO bridge must advertise the VTF MIME type before it will dispatch `.vtf` files to the Qt plugin. The installer requires exactly one known metadata span in each bridge DLL and stops with an error if the expected layout is not found.

## Usage

### Open a VTF

1. Choose **File > Open**.
2. Select a `.vtf` file.
3. Krita imports the full-resolution image from the first frame and face.

### Save a VTF

1. Choose **File > Save As**.
2. Enter a filename ending in `.vtf`.
3. Choose the version, image format, mipmaps, thumbnail, bump-map scale, and flags in the **VTF Export Options** dialog.
4. Resolve any validation message.
5. Select **OK** to write the file.

The last accepted export settings are retained for the next VTF export.

## Building

The Windows workflow is defined in `.github/workflows/windows.yml`. It:

1. Downloads the maintained Krita Qt 5 dependency environment.
2. Downloads LLVM MinGW.
3. Configures the standalone Qt image plugin with CMake and Ninja.
4. Builds `kimg_vtf.dll`.
5. Builds and runs the codec tests.
6. Packages the plugin, MIME description, installer, and README as `krita-vtf-windows`.

The verified CI output is committed at `bin/imageformats/kimg_vtf.dll`, making the repository ZIP directly installable. Maintainers should replace the bundled DLL after successful source changes and verify that its SHA-256 matches the corresponding CI artifact.

The standalone build definition is `src/CMakeLists.qimageio.txt`. The plugin links against Qt 5 Core, Gui, and Widgets rather than Krita's private libraries.

## Tests

Codec tests cover:

- VTF minor versions 7.0 through 7.5.
- Compressed and uncompressed output formats.
- Header version and image-format fields.
- Codec round trips.
- Mipmap counts and `NOMIP` synchronization.
- Thumbnail header fields.
- Invalid version and non-power-of-two mipmap rejection.

GitHub Actions runs the tests with:

```powershell
ctest --test-dir C:\vtf-build --output-on-failure
```

## Current limitations

- Import uses only the first frame and first face.
- Export writes one frame, one face, and depth 1.
- Animated textures, cubemaps, and volume textures are not exported.
- Import returns only the largest mip level to Krita.
- The DXT encoder prioritizes deterministic interoperable output rather than maximum compression quality.
- Installation patches Krita's QImageIO bridge metadata and may need to be repeated after updating or reinstalling Krita.
- The installer currently targets Windows and a Qt 5 Krita build.

Unsupported structures are not silently generated. Multi-frame, cubemap, and volume options are omitted until their data layout is implemented and tested.

## Troubleshooting

### The installer says Krita is running

Close all Krita windows and run the installer again. The bridge DLLs cannot be safely replaced while Krita is active.

### The installer cannot find Krita

Pass the actual installation directory:

```powershell
.\install.ps1 -KritaRoot "C:\Path\To\Krita"
```

The directory must contain `bin\krita.exe`.

### The metadata span was not found

The installed Krita version has a different QImageIO bridge layout. Do not modify the DLL manually or force the installation. Open an issue with the Krita version and installer error.

### VTF is missing from the file dialog

Confirm that:

- Krita was restarted after installation.
- `kimg_vtf.dll` exists under `bin\imageformats`.
- `%LOCALAPPDATA%\mime\packages\vtf.xml` exists.
- The installer completed without an error.

## License

GPL-2.0-or-later.
