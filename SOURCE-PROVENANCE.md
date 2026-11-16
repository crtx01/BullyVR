# Provenance and licensing inventory

The current APK is a recovered binary plus an additive source-built JavaScript
runtime. It is not a complete source reconstruction of the original game or of
the exact recovered emulator build.

| Component | Source status |
|---|---|
| Current Recovery+ review layer | Included in `src/recoveryplus` |
| Staging, tests and PPC payload generators | Included in `tools` |
| Original r22 runtime logic | Readable reference included; embedded assets omitted |
| Earlier native Bully VR code | Included in `src`, not verified identical to recovered r22 |
| DolphinXR upstream | Public upstream and commit recorded in BUILD.md |
| Exact recovered libmain/libbully_recovery binaries | Retained by packaging from a supplied APK; corresponding build provenance remains incomplete |
| Bully game code/content and player saves | Not distributed |

Dolphin's licensing statement and license texts are preserved under `upstream/`.
Retain original per-file notices and relevant dependency licenses. Earlier native
code references UltimateXR hands and ImGui; no third-party art is bundled here.
The source snapshot does not, by itself, establish that all corresponding-source
obligations for the recovered APK have been satisfied. Reconstructing that exact
native build remains a separate task; do not label this as a fully reproducible
from-source APK release.

Recorded libmain SHA256:
`0c779f7939b6790e9e06678d02e7f518987906f3a8bb290a0ae98b575c180262`

Recorded original runtime SHA256:
`7951bac83a9bf1d7504b4c3b8ae937462093aeaa38bd4c5405ac7d8e1155d1b7`
