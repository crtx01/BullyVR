# Bully VR — experimental Quest adaptation

Created by **crtx01**, a Brazilian developer. This source snapshot documents the
Recovery+ r40 test candidate and the available earlier native implementation.
It is an independent fan project, not an official Rockstar or Meta release.

**Published APK:** [Recovery+ r40 test update](https://github.com/crtx01/BullyVR/releases/tag/v0.0.1-r40).
The current source and test APK are **r40**, November 24. This update fixes a
reproduced VR-menu font-texture assertion. See [STABILITY.md](STABILITY.md) for
measured coverage and remaining validation limits.
Sideload the APK and open **BullyVR Recovery+** under Unknown Sources. Supply your
own US Wii copy of Bully: Scholarship Edition (**RB7E54**) in a **Bully VR** folder,
then select that folder through Dolphin's Add Games/Add folder picker.
Fresh VR menus default to English; Portuguese is available in settings.
Updating an installation signed with the same key preserves its existing saves.
See [CONTROLS.md](CONTROLS.md) for current bindings.

The runtime integrates tracked VR hands, gesture punches, head-oriented movement,
VR menus, equipment controls, cheats, locations and mission controls into a
recovered Dolphin-based Quest build. These are development features; this snapshot
does not establish full campaign compatibility or crash-free operation.

## Source layout

- `src/recoveryplus/`: current additive runtime fixes, including controls, fists,
  transition filtering, tutorial placement and chemistry timing.
- `tools/`: staging, packaging, PPC payload generators and regression checks.
- `src/*.cpp` and `src/*.h`: available earlier native implementation. This is
  **not established as the exact source of the recovered r22 native binary**.
- `reference/recoveryplus-r22-runtime-readable.js`: recovered baseline logic,
  with embedded font/texture data omitted.
- `Documentation/recoveryplus/mission-catalog.json`: mission/class catalogue.
- `tests/`: earlier native-unit checks, not proof of current headset gameplay.

Read [BUILD.md](BUILD.md), [SOURCE-PROVENANCE.md](SOURCE-PROVENANCE.md) and
[STABILITY.md](STABILITY.md) before building or testing. No game disc, creator
save, signing key, credentials or device backup is included.

## Português

Projeto experimental criado por **crtx01 (Brasil)**. Este pacote disponibiliza o
código existente, testes e processo de montagem. A build Recovery+ depende de
bibliotecas recuperadas de um APK anterior; não afirmamos que todo o APK possa
ser recompilado do zero apenas com este repositório. A revisão atual inclui uma correção
para a falha de textura de fonte reproduzida no menu VR do Quest. Cada jogador fornece sua
própria cópia do jogo.

Feedback: open a GitHub issue with the build, Quest model, action and crash log,
or contact **davidperetta12@gmail.com**. Full campaign validation is still pending.
