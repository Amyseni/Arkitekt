# What is Arkitekt?

Arkitekt is an API/wrapper building on top of DynASM to allow emitting instructions from files that have not been preprocessed. This is achived through a mixture of concepts, constraints, `if constexpr`, and templating to allow each supported instruction to essentially be represented by an empty consteval struct containing an `operator()` which uses the aforementioned concepts and constraints to cleanly define exactly what types it can be called with. With most instructions simply using the universal `Operand` constraint, however some more limited/nuanced instructions like `ret` may only accept `imm32/64` for immediate values. 

It also contains a wrapper generator function which can be called on a fully typed function pointer (currently x86_32 only) and will return a function pointer that can be used with a detouring library to hook the function in question. The goal is to expand the supported instructions and architectures and eventually include helper functions for common instruction patterns and a system of linking defined/hooked functions to allow the assembler to generate runtime assembly from user created scripts. Essentially turning a reverse engineered binary and its functions into a ready-to-use scripting API. Maybe I'll even finish it before I kick it

# Special Thanks / Libraries Used

Arkitekt uses some code from and is otherwise inspired by LibZHL (from an alternate timeline where it was finished :3) by Kilburn (@FixItVinh)

Arkitekt also owes nearly its entire existence to the team behind FTL Hyperspace for not just their ZHL modifications but their amazing willingness to let me bug them with weird esoteric issues and help anyways, as well as Aurie/YYToolkit and their author Archie (@[archie_osu](https://github.com/archie-osu))
HDE (Hacker's Disassembler) and MologieDetours are also used to create and install function hooks. Their licenses are included with them (in detours.h for Mologie, and in hde32/LICENSE and hde64/LICENSE for HDE).

- LibZHL is Licensed under [MIT](https://github.com/amyseni/arkitekt/blob/master/LibZHL-License) and Arkitekt.cpp and Arkitekt.h must maintain attribution to Kilburn (@FixItVinh) if used.
- Hyperspace is Licensed under CC-BY-SA 4.0
- Aurie and YYToolKit are Licensed under AGPL-3.0
- DynASM is Licensed under [MIT](https://www.opensource.org/licenses/mit-license.php)
