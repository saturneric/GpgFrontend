# Bundled Fonts

## JetBrains Mono NL 2.304

- Upstream: <https://github.com/JetBrains/JetBrainsMono>
- Release: <https://github.com/JetBrains/JetBrainsMono/releases/tag/v2.304>
- Archive: `JetBrainsMono-2.304.zip`,
  sha256 `6f6376c6ed2960ea8a963cd7387ec9d76e3f629125bc33d1fdcd7eb7012f7bbf`
- License: SIL Open Font License 1.1, see `OFL.txt` in this directory.

Files taken verbatim from `fonts/ttf/` inside that archive:

| File                          | sha256                                                             |
| ----------------------------- | ------------------------------------------------------------------ |
| `JetBrainsMonoNL-Regular.ttf` | `fb3b2575d7b0657359707993288f12a7360344d39387bb26050e276d61f6bd2a` |
| `JetBrainsMonoNL-Bold.ttf`    | `0198e841824025f8876e5c297f0b9b497ee8d6eb9969710a3328e1303f996ec3` |
| `OFL.txt`                     | `30f0c136e3c88e422d0791acd97238870f9054a9729bc34cf2ff0d4ed8cac4ad` |

### Do not modify these files

OFL-1.1 section 5 permits the Reserved Font Name "JetBrains Mono" only on
unmodified originals. Subsetting, re-hinting or any other transformation would
make a derivative work that has to be renamed, which defeats the point of
bundling a font whose name the code looks up by. If the binary size ever needs
to come down, the lever is rcc's compression settings, never the font bytes.

### Why the NL cut, and why static TTFs

`NL` is the no-ligature cut. Stock JetBrains Mono ligates `->`, `--` and `==`,
which fires inside `-----BEGIN PGP MESSAGE-----` and throughout armored
payloads. Advance widths are identical between the two cuts, so column
alignment is unaffected. The only QFont-level way to suppress ligatures below
Qt 6.7 is `QFont::PreferNoShaping`, which would also disable the shaping that
Arabic and Hebrew need, so the no-ligature files are the portable answer.

The static hinted TTFs are shipped rather than the variable font: Qt 5.15 has
no variable-axis support at all, and Qt 6 gained `QFont::setVariableAxis` only
in 6.7, so a variable file would register its default instance and synthesise a
fake bold instead of using the real Bold master.

### How they reach the application

`resource/qrc/fonts.qrc` compiles them into `libgf_res` under the `/fonts`
prefix; `GpgFrontend::UI::RegisterBundledFonts()` (see
`src/ui/function/AppearanceFont.cpp`) hands them to `QFontDatabase` once, from
the `GpgFrontendApplication` constructor. Because they live in the qrc rather
than on disk, no macOS `Info.plist` `ATSApplicationFontsPath` entry, no
`Contents/Resources` copy and no sandbox entitlement is involved.
