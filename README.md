# VTF import/export for Krita

Adds Valve Texture Format (`.vtf`) to Krita's standard **File > Open** and
**File > Save As** workflows on Windows.

The project uses a standard Qt 5 `QImageIOPlugin`, then enables the VTF MIME in
Krita's existing QImageIO import/export bridge. This avoids linking against
Krita's private C++ ABI.

## Format support

Import:

- VTF 7.x, first frame and first face
- RGBA8888, ABGR8888, RGB888, BGR888, RGB565
- I8, IA88, A8, ARGB8888, BGRA8888, BGRX8888
- DXT1, DXT1 one-bit alpha, DXT3, DXT5
- Full-resolution mip level

Export:

- VTF 7.2
- RGBA8888
- One frame, one mip level, no thumbnail

## Install

1. Download the `krita-vtf-windows` artifact from the latest successful
   **Windows native plugin** GitHub Actions run.
2. Extract the artifact.
3. Fully close Krita.
4. Run PowerShell as Administrator:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install.ps1
```

The installer:

- Installs `kimg_vtf.dll` into `bin\imageformats`.
- Registers the VTF MIME for the current Windows user.
- Makes a backup of Krita's QImageIO bridge DLLs.
- Applies a byte-length-preserving metadata patch so Krita advertises and
  dispatches VTF through its existing QImageIO bridge.

Restart Krita after installation. `.vtf` then works through **File > Open** and
**File > Save As**.

## CI

GitHub Actions downloads Krita's official Windows Qt 5 dependency environment,
uses the same LLVM MinGW ABI family, and builds the standalone Qt image plugin.

License: GPL-2.0-or-later.
