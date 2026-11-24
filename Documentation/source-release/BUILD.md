# Building the Recovery+ additive candidate (Windows)

## Prerequisites

Python 3.12, Node.js, Android SDK build-tools 36.0.0, Android NDK
29.0.14206865 (ARM64 callback test), and a local Android debug keystore.
Install development packages with `python -m pip install -r requirements-dev.txt`.
The prompt generator uses the locally installed Windows Arial Bold font.

You must supply the **original Recovery+ r22 APK**. A newer r33/r34 APK cannot
substitute for it. It is not included in this source archive. The preparation
script verifies the native engine and original script against recorded SHA256s.

```powershell
python tools/prepare_recoveryplus_source.py C:/path/to/RecoveryPlus-original.apk
python tools/recoveryplus_prompt_pack.py
python tools/build_recoveryplus_mission_skip.py
python tools/build_recoveryplus_tutorial_layout.py
$env:BULLY_REVIEW_REVISION='r40'
$env:BULLY_REVIEW_DATE='2026-11-24'
python tools/recoveryplus_stage_review.py
node --check build/recoveryplus-review-runtime.js
node tools/check_recoveryplus_commands.js
node tools/check_recoveryplus_transitions.js
node tools/check_recoveryplus_travel.js
node tools/check_recoveryplus_initialization.js
node tools/check_recoveryplus_fists.js
node tools/check_recoveryplus_help_distance.js
node tools/check_recoveryplus_input_transitions.js
node tools/check_recoveryplus_menu_settle.js
node tools/check_recoveryplus_tutorial_slot.js
node tools/check_recoveryplus_draw_distance.js
python tools/check_recoveryplus_mission_skip.py
python tools/check_recoveryplus_streaming_guard.py
python tools/build_recoveryplus_review.py --recoveryplus
```

Output: `Builds/RecoveryPlus-2026-11-24/BullyVR-RecoveryPlus-r40.apk`.
Packaging checks signature, ZIP CRC, DEX checksums and 16-KB library alignment.
Signing with your own key changes the APK hash and may prevent an in-place update
over an APK signed by someone else. Back up your saves before changing signatures.

## Earlier native C++ code

The earlier implementation uses `src/CMakeLists.txt`, Android NDK and ImGui from
the DolphinXR checkout at `upstream/`. The recorded upstream is
https://github.com/iChris4/dolphinXR at
`6955095298af7886f71251558d3d2d4ac471c7b3`.
Initialize that checkout's submodules before attempting a native build.
`reference/upstream-local-changes.patch` records the local engine source change.
This path is provided for inspection and continued development; it does **not**
reproduce the Recovery+ r22 native binary. Art/model/font assets are not included.

The local checks cannot reproduce Android graphics-driver execution, head tracking,
native texture-cache corruption or the reported teleport crash without a device.

For the optional ADB test helpers, set `ANDROID_SERIAL` to your own Quest's
identifier from `adb devices`. They also require a Frida server you supply on
the device and explicit setup of the local port forward. State-load/save helpers
additionally require the locally built Android state agent; no device agents,
personal checkpoints or test saves are distributed in this source archive.
