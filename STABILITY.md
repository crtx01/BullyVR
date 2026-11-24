# Recovery+ r39/r40 — November 24, 2026

## Reproduced failure and fix

The r38 on-headset menu sweep stopped at an ImGui assertion in `GetTexID`:
a draw command referenced font texture data whose GPU texture had not been
created yet. A modal alert then stopped guest progress, although the process
and VR compositor were still running. This must not be counted as a passed test.

VR overlay glyphs can be appended after OnScreenUI's usual texture-upload pass.
`src/recoveryplus/imgui-textures.js` calls the existing native texture updater
on the video thread before that draw command resolves its texture reference.
It does not suppress the assertion, fabricate texture handles or swallow crashes.
`reference/upstream-local-changes.patch` includes the equivalent readable fix
in DolphinXR's `OnScreenUI.cpp`.

The callback is pinned to the recorded Recovery+ engine layout and checks the
intercepted ARM64 instruction before attaching. Other engine binaries require
their own layout verification.

## On-headset observations

The r39 repeat completed three passes through six VR tabs, with repeated row
navigation and help open/close. The late-texture uploader handled pending font
requests with zero unresolved references in that run. A subsequent sweep attempted
all 92 equipment indices. Some props are not owned/equippable through this native
inventory route; a rejected selection does not validate a model or weapon use.

A second native grant/equip pass covered all 91 non-fist indices and recorded
17 successful equipment-ID confirmations with real eye-frame captures. Remaining
indices were rejected or not equippable through that route. Trigger requests in
the first sweep are not visual confirmation of firing, hit direction or damage.
Native population profile values read back as 220/120/200 in each of the three
profile copies. Far clip read back as 650, preserving reflection exclusions.

No crash-free campaign or long-session guarantee is claimed. The earlier native
texture-cache invalidation crash is a separate failure without a proven general
fix. Device history also contains focus-change ANRs and low-memory termination.

## Other changes

- Release review control overrides while the native pause/menu is active.
- Classify the actual held weapon when its selected menu row is stale.
- Delay cheat synchronization until 750 ms of uninterrupted gameplay after menus,
  classes, cinematics and travel. Existing settings remain pending until safe.
- Keep shared-code-slot ownership checks and empty-hands acknowledgement.
- r40 scales only the controls card's generated vertices to 68% around the
  viewport center. This replaces the earlier HUD-depth adjustment, which did not
  adequately change the card. Header, text and close instruction scale together;
  indices, texture refs and other overlay groups are preserved. PT/EN instructions
  now match X pause and Y grab. User comfort still needs direct assessment.
- Keep the bounded 650 far-plane floor and reflection protection. This does not
  establish that all tree LODs, streaming distances or NPC retention are fixed.
- Preserve existing sky/EFB/HUD filtering. No all-culling bypass is enabled.

## Local regressions

The compiled ARM64 guard matrix checks 8,464 ordered equipment pairs, including
cooldown, loading, class, cinematic and travel gates. The staged menu test visits
35 location/mission pages and 254 rows. Input transition, fists, menu-settle,
tutorial-slot, help-distance, command and PPC calling convention tests exercise
specific logic; they are not full gameplay emulation.

Public CI runs the source tests which do not require the private original APK.
The full candidate build and remaining tests require that verified base and
Android tooling, as documented in BUILD.md.
