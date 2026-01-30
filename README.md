# What is Arkitekt?

Arkitekt is a function hooking library for x86-32 Windows games (currently) with a goal to eventually support 32-and 64-bit Windows and Linux games and a specific interest in supporting GameMaker Studio 2 games. Its goal is to make it as easy as possible to define a function or variable that exists within your game's binary, and then bind and hook them seamlessly. Reading, writing, overwriting and patching them as you see fit.

One of the core benefits of the way it binds those functions and variables is that they are constinit-friendly. Meaning that if a bound function is static, then it is guaranteed to be callable and its binding object will never be uninitialized.

# Special Thanks / Libraries Used

Arkitekt uses some code from and is otherwise heavily based on LibZHL by Kilburn (@FixItVinh), as well as FTL Hyperspace (and specifically their own LibZHL modifications/upgrades) and Aurie/YYToolKit.
HDE (Hacker's Disassembler) and MologieDetours are also used to create and install function hooks. Their licenses are included with them (in detours.h for Mologie, and in hde32/LICENSE and hde64/LICENSE for HDE).

- LibZHL is Licensed under MIT 2.0
- Hyperspace is Licensed under CC-BY-SA 4.0
- Aurie and YYToolKit are Licensed under AGPL-3.0
