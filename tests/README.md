The C++ checks cover inventory input transitions, native trigger release,
locomotion, HUD panel selection and existing contact/menu rules.

`physical_bridge.py` runs the generated PPC instructions in Unicorn, including
the native projectile return path. It checks stack arguments, NPC isolation,
zero-velocity drops, the current player root and the player-only visibility gate.
Install its development dependency with `python -m pip install -r tests/requirements.txt`.
Run `python tools/build_contact_bridge.py` before it when changing the generator.

These are regression checks for code and calling conventions. They do not
validate Quest tracking comfort, actual Wii attack animations, the renderer,
or completion of gameplay missions. Those require tests on the headset.
