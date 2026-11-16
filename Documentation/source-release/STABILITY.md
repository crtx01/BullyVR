# r34 local stability review — 2026-11-16

## Changes

- Cancel pending carnival admission/teleport when gameplay changes to a mission,
  class, cinematic, another travel request or equipment operation.
- Revalidate admission player/mailbox spans against actual MEM1/MEM2 bounds.
- Recheck mission confirmation against loading, class and equipment state.
- Prevent a partial initialization failure from installing duplicate hooks every
  100 ms. A failed initialization requires restarting the app; inspect the log.
- Bound fists player/inventory/item/mailbox reads and expire retries even when
  player state is unavailable. Null held-item pointers count as empty hands.
- Cancel pending mission completion on transition or changed mission. Reject
  invalid Gecko-handler magic below as well as above the supported range.
- Retain r33 serialized equipment/travel protections.

## Available crash evidence

An older November 11 SIGILL jumped to the start of libmain.so (ELF header).
A November 13 SIGSEGV occurred in texture-cache invalidation at native offset
0x8410e8, called from cleanup at 0x840dfc. These are distinct failures.
Neither log proves the cause of the user's subsequent r33 crashes. The native
texture-cache crash has **not** been reproduced or conclusively fixed by r34.
No bypass of cache cleanup or suppression of native crashes is installed.

## Validation scope

Regression checks exercise real staged travel logic, one-shot initialization,
fists bounds/acknowledgement and actual compiled ARM64 equipment callbacks.
PPC mission-completion calling convention and transition/command tests also run.
These checks are not a full engine emulator or an on-headset gameplay test.
No Quest was available for this revision. Treat r34 as a candidate, not stable.

When a Quest is available, capture `adb logcat -b all -d` immediately after a
crash, noting build, weapon, destination and whether a mission/cutscene was active.
Redact account/device details before sharing logs publicly.
