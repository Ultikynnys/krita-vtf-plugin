# Native VTF import/export for Krita

Native `KisImportExportFilter` plugins that add Valve Texture Format (`.vtf`)
to Krita's standard **File > Open** and **File > Save As** workflows.

## Current format support

Import:

- VTF 7.x, first frame and first face
- RGBA8888, ABGR8888, RGB888, BGR888, RGB565
- I8, IA88, A8, ARGB8888, BGRA8888, BGRX8888
- DXT1, DXT1 one-bit alpha, DXT3, DXT5
- Reads the full-resolution mip level

Export:

- VTF 7.2
- RGBA8888
- One frame, one mip level, no thumbnail

The exporter intentionally starts with a lossless, uncompressed profile. DXT
compression and configurable texture flags can be added without changing the
native Krita integration.

## Download and install

1. Open the latest successful **Windows native plugin** workflow run.
2. Download the `krita-vtf-windows` artifact.
3. Extract it.
4. Close Krita.
5. Run PowerShell as Administrator:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install.ps1
```

The DLLs are installed into `lib\kritaplugins`, where Krita discovers native
file filters at startup. The MIME definition is installed under
`share\mime\packages`.

## Development

The plugin is built inside a matching Krita source checkout because Krita does
not publish a standalone Windows plugin SDK. GitHub Actions obtains Krita's
official dependency environment, injects `src/` into `plugins/impex/vtf`, and
builds only `kritavtfimport` and `kritavtfexport`.

License: GPL-2.0-or-later.
