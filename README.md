# What is Arkitekt?

Arkitekt is an API/wrapper building on top of DynASM to allow calling its emitter functions from a much more readable C++ syntax, rather than preprocessing any file that emits code. Currently supports Win32 only, but plans to expand to MacOS/Linux and Windows 32-64bit (and ARM for MacOS). <br><br>
It implements macros that can be used to detour functions in a running game in such a way that safe code execution can be achieved from only a function pointer and accompanying C++ typedef. Including a very helpful wrapper back to the original function that ensures it has a safe context to run whether it gets passed the original arguments or any arguments you choose, and can be skipped if the desire is to entirely overwrite a hooked function.

Use patterns can be seen in [Organik, a mod I make for Synthetik: Legion Rising](https://github.com/amyseni/organik) but, there's not really a tutorial yet.

# Special Thanks / Libraries Used / Licensing

Arkitekt uses some code from and is otherwise inspired by LibZHL (from an alternate timeline where it was finished :3) by Kilburn (@FixItVinh)

Arkitekt also owes nearly its entire existence to the team behind FTL Hyperspace for not just their ZHL modifications but their amazing willingness to let me bug them with weird esoteric issues and help anyways, as well as Aurie/YYToolkit and their author Archie (@[archie_osu](https://github.com/archie-osu))
HDE (Hacker's Disassembler) and MologieDetours are also used to create and install function hooks. Their licenses are included with them (in detours.h for Mologie, and in hde32/LICENSE and hde64/LICENSE for HDE).

- LibZHL is Licensed under [MIT](https://github.com/amyseni/arkitekt/blob/master/LibZHL-License) and Arkitekt.cpp and Arkitekt.h must maintain attribution to Kilburn (@FixItVinh) if used.
- Hyperspace is Licensed under CC-BY-SA 4.0
- Aurie and YYToolKit are Licensed under AGPL-3.0
- MologieDetours' license is included with it in [detours.h](https://github.com/amyseni/arkitekt/blob/master/src/detours.h) and also uses HDE (Hackers Disassembler) which has its license included in [hde(32/64)/LICENSE](https://github.com/amyseni/arkitekt/blob/master/src/hde32/LICENSE).
- DynASM is Licensed under [MIT](https://www.opensource.org/licenses/mit-license.php)
