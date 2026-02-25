# AGENTS.md — Animal Crossing GameCube PC port

## Project Overview

This repository is a work-in-progress PC port of Animal Crossing (GameCube).
It is based on a complete matching decompilation of the game and is currently in progress of being ported from the PowerPC/MWCC/GC platform to x86 (not amd64 yet) clang/msvc/gcc and windows/linux.

### Supported Game Versions

| ID           | Region      |
|--------------|-------------|
| `GAFE01_00`  | USA Rev 0 (default) |
| `GAFU01_00`  | Australia Rev 0     |

---

## Repository Layout

```
forest/
├── configure.py          # Master build configuration script (generates build.ninja)
├── build.ninja           # Generated ninja build file
├── compile_commands.json # Compilation database for IDE/tooling support
├── objdiff.json          # Configuration for objdiff (binary diffing tool)
│
├── config/
│   ├── GAFE01_00/        # USA version configuration
│   │   ├── config.yml    # dtk decomp config (DOL hash, symbol/split info, relocations)
│   │   ├── symbols.txt   # Known symbol addresses
│   │   ├── splits.txt    # Section split definitions
│   │   ├── build.sha1    # Expected output hash for verification
│   │   ├── ldscript.tpl  # Linker script template
│   │   └── foresta/      # REL module configuration (the main game REL)
│   └── GAFU01_00/        # Australia version configuration
│
├── orig/
│   ├── GAFE01_00/        # Original disc image files (user-supplied, not in repo)
│   └── GAFU01_00/
│
├── build/                # Build output directory
│   ├── compilers/        # Downloaded Metrowerks CodeWarrior compilers
│   ├── binutils/         # Downloaded GNU binutils for PowerPC
│   ├── tools/            # Downloaded decomp tools (dtk, sjiswrap, objdiff-cli, orthrus)
│   └── GAFE01_00/        # Per-version build artifacts
│
├── assets/               # Project assets (e.g. objdiff screenshot)
│
├── include/              # All header files
├── src/                  # All source files
└── tools/                # Python build tooling and utilities
```

---

## Source Code Structure (`src/`)

The source is split into two major linkage units: the **static DOL** (the main executable) and the **foresta REL** (a relocatable module loaded at runtime containing all game logic).

### Static DOL (`src/static/` and some top-level `src/` files)

The DOL contains system-level code, SDK libraries, and the boot/init path.

| Directory / File                 | Description |
|----------------------------------|-------------|
| `src/static/boot.c`             | System bootstrap and initialization |
| `src/static/jsyswrap.cpp`       | JSystem wrapper layer for C code |
| `src/static/version.c`          | Version identification |
| `src/static/initial_menu.c`     | Initial menu screen |
| `src/static/dvderr.c`           | DVD error handling |
| `src/static/bootdata/`          | Boot data (logo textures, window data) |
| `src/static/nintendo_hi_0.c`    | Nintendo logo data |
| `src/static/GBA2/`              | GBA Joy Boot link support |
| **`src/static/libforest/`**     | **Forest engine library** |
| `src/static/libforest/emu64/`   | N64-to-GC graphics command emulation layer (emu64) |
| `src/static/libforest/osreport.c` | OS report wrapper |
| `src/static/libforest/fault.c`  | Crash/fault handler |
| `src/static/libforest/ReconfigBATs.c` | BAT register reconfiguration |
| `src/static/libu64/`            | N64 compatibility library (debug, gfxprint, pad) |
| `src/static/libc64/`            | C64 utility library (malloc, math, printf, random) |
| `src/static/libultra/`          | N64 Ultra SDK reimplementation for GameCube |
| `src/static/libultra/gu/`       | Graphics utility math functions (matrix, trig, etc.) |
| `src/static/libjsys/`           | JSystem wrapper extensions |
| `src/static/dolphin/`           | **Dolphin SDK** (Nintendo's official GC SDK) |
| `src/static/dolphin/ai/`        | Audio interface |
| `src/static/dolphin/ar/`        | ARAM (auxiliary RAM) |
| `src/static/dolphin/card/`      | Memory card |
| `src/static/dolphin/db/`        | Debug |
| `src/static/dolphin/dsp/`       | DSP (audio processor) |
| `src/static/dolphin/dvd/`       | DVD drive |
| `src/static/dolphin/exi/`       | Expansion interface |
| `src/static/dolphin/gba/`       | GBA link |
| `src/static/dolphin/gx/`        | **Graphics (GX)** — the GC's GPU API |
| `src/static/dolphin/mtx/`       | Matrix/vector math |
| `src/static/dolphin/os/`        | OS services (threads, interrupts, cache, memory, RTC, etc.) |
| `src/static/dolphin/pad/`       | Controller input |
| `src/static/dolphin/si/`        | Serial interface |
| `src/static/dolphin/vi/`        | Video interface |
| `src/static/dolphin/OdemuExi2/` | DEV debug interface |
| `src/static/JSystem/`           | **JSystem** libraries (J2D, J3D, JFramework, JKernel, JUtility, etc.) |
| `src/static/Famicom/`           | NES/Famicom emulator (for playable NES games) |
| `src/static/jaudio_NES/`        | NES audio system (jaudio) |
| `src/static/MSL_C.PPCEABI.bare.H/` | Metrowerks Standard Library (C runtime) |
| `src/static/Runtime.PPCEABI.H/` | PowerPC EABI runtime support |
| `src/static/TRK_MINNOW_DOLPHIN/` | Target Resident Kernel (debugger support) |

### Foresta REL — Game Logic (`src/` top-level and subdirectories)

The REL module contains all actual game logic. It is loaded at runtime by the DOL.

| Directory / File              | Description |
|-------------------------------|-------------|
| `src/main.c`                  | Entry point (`mainproc`): creates threads for graphics, pad, IRQ |
| `src/graph.c`                 | Graphics thread: manages render pipeline, display lists, double buffering |
| `src/game.c`                  | Core game loop, frame management, game state |
| `src/executor.c`              | REL prolog/epilog (static constructors/destructors, `_unresolved`) |
| `src/audio.c`                 | Audio interface for game (wraps jaudio_NES) |
| `src/padmgr.c`               | Pad (controller) manager thread |
| `src/irqmgr.c`               | IRQ (interrupt request) manager |
| `src/famicom_emu.c`           | Famicom/NES emulator integration |
| `src/zurumode.c`              | Debug/cheat mode |
| `src/first_game.c` / `src/second_game.c` | Game phase initialization |
| `src/player_select.c`         | Player selection screen |
| `src/save_menu.c`             | Save menu |
| `src/gamealloc.c` / `src/gfxalloc.c` | Game and graphics memory allocators |
| `src/THA_GA.c` / `src/TwoHeadArena.c` | Two-headed arena memory allocators |
| `src/lb_reki.c` / `src/lb_rtc.c` | Calendar/RTC (real-time clock) library |
| `src/c_keyframe.c`            | Keyframe animation |
| `src/PreRender.c`             | Pre-render buffer management |
| `src/evw_anime.c`             | Event animation |
| `src/ev_cherry_manager.c`     | Cherry blossom event manager |
| `src/f_furniture.c`           | Furniture system |
| `src/s_cpak.c`                | Controller pak support |

#### `src/game/` — Game Systems (m_ prefix)

The `m_` prefix stands for "module" and contains the bulk of game systems:

| File Pattern                  | Description |
|-------------------------------|-------------|
| `m_actor*.c`                  | Actor (entity) system: spawn tables, shadows, types |
| `m_player*.c`                 | Player logic: movement, items, actions, animations (huge — many `.c_inc` sub-files) |
| `m_scene*.c` / `m_game_dlftbls.c` | Scene management and game state tables |
| `m_collision_bg*.c` / `m_collision_obj.c` | Collision detection (background grid, objects) |
| `m_field_*.c`                 | Field/overworld management (assessment, info, generation) |
| `m_npc*.c`                    | NPC scheduling, walking, behavior |
| `m_event*.c`                  | Event system and event-map NPC management |
| `m_common_data.c`             | Shared global game data |
| `m_controller.c`              | Controller input abstraction |
| `m_camera2.c`                 | Camera system |
| `m_msg*.c`                    | Message/dialog system (text boxes, cursors, animations) |
| `m_choice.c`                  | Multiple-choice dialog |
| `m_play.c` / `m_play_h.c`    | Play state (the main in-game state) |
| `m_home.c` / `m_house.c`     | House/home interior management |
| `m_land*.c`                   | Town/land data management |
| `m_lights.c`                  | Lighting system |
| `m_view.c`                    | View/viewport setup |
| `m_skin_matrix.c`             | Skinned mesh matrix calculation |
| `m_time*.c`                   | Time system and time-input overlay |
| `m_card.c`                    | Memory card save/load |
| `m_mail*.c`                   | Mail/letter system |
| `m_quest*.c`                  | Quest/errand system |
| `m_shop.c`                    | Shop system |
| `m_museum*.c`                 | Museum display system |
| `m_bg_item.c` / `m_bg_tex.c` | Background items and textures |
| `m_submenu*.c`                | Submenu/inventory overlay |
| `m_font*.c`                   | Font rendering |
| `m_fbdemo*.c`                 | Framebuffer demo effects (fades, wipes, triforce transition) |
| `m_debug*.c`                  | Debug utilities and debug mode |
| `m_*_ovl.c`                   | Overlay modules (dynamically loaded game sub-systems) |
| `m_titledemo.c` / `m_trademark.c` | Title screen demo and trademark display |
| `m_bgm.c`                     | Background music control |
| `m_vibctl.c`                  | Vibration/rumble control |
| `m_kankyo.c`                  | Environment/weather system |
| `m_island.c`                  | Island (tropical island sub-area) |
| `m_prenmi.c`                  | Pre-NMI (reset button) handling |
| `m_private.c`                 | Private/personal data |
| `m_diary*.c`                  | Diary system |
| `m_lib.c` / `m_olib.c`       | Math/utility libraries |
| `m_string.c`                  | String processing |
| `m_rcp.c`                     | RCP (Reality Co-Processor) display list setup |
| `m_roll_lib.c`                | Credits roll library |
| `sys_*.c`                     | System utilities (math, matrix, stacks, ucode, vimgr) |

#### `src/actor/` — Actors (Game Entities)

All in-game entities (actors) with the `ac_` prefix. Each actor typically has `_move.c_inc` (logic) and `_draw.c_inc` (rendering) companion files.

| Subdirectory / Pattern         | Description |
|--------------------------------|-------------|
| `ac_*.c`                       | Individual actor implementations (weather, structures, items, vehicles, etc.) |
| `src/actor/npc/`              | NPC actors: villagers (`ac_npc.c`, `ac_npc2.c`), special NPCs (shop masters, guides, police, etc.), event NPCs (festival/holiday participants) |
| `src/actor/npc/event/`        | Event-specific NPC actors (santa, artist, ghost, gypsy, etc.) |
| `src/actor/tool/`             | Held tool actors (umbrella, flags, hats, etc.) |

#### `src/furniture/` — Furniture Actors

Hundreds of individual furniture piece implementations (`ac_sum_*.c`, `ac_ike_*.c`, `ac_nog_*.c`, etc.), organized by creator/category prefix.

#### `src/bg_item/` — Background Items

Background placed items (ground decorations, seasonal items, etc.).

#### `src/effect/` — Visual Effects

All particle and visual effects (`ef_*.c`): dust, footprints, weather particles, sparkles, expressions, fireworks, etc.

#### `src/data/` — Data Tables

Static data tables for the game: field layouts, font data, item definitions, NPC data, model data, scene configs, player data, submenu data, titledemo data.

---

## Header / Include Structure (`include/`)

| Directory                      | Description |
|--------------------------------|-------------|
| `include/dolphin/`            | Dolphin SDK headers (GX, OS, DVD, PAD, VI, AI, AR, DSP, EXI, SI, Card, MTX) |
| `include/libforest/`          | Forest engine headers |
| `include/libforest/emu64/`    | **emu64** — N64 display list interpreter that translates N64 GBI commands to GC GX calls |
| `include/libu64/`             | N64 compatibility layer headers |
| `include/libultra/`           | Reimplemented N64 Ultra SDK headers (threads, messages, timers, controllers, math) |
| `include/libjsys/`            | JSystem wrapper headers |
| `include/libc64/`             | (in `src/static/libc64/`) C utility headers |
| `include/JSystem/`            | JSystem library headers (J2D, J3D, JFramework, JKernel, JUtility, JSupport) |
| `include/Famicom/`            | NES emulator headers |
| `include/jaudio_NES/`         | NES audio engine headers |
| `include/PR/`                 | N64 SDK public headers (`gbi.h` — Graphics Binary Interface, `abi.h` — Audio Binary Interface) |
| `include/GBA/` / `include/GBA2/` | GBA link headers |
| `include/MSL_C/`              | Metrowerks Standard Library C headers |
| `include/MSL_CPP/`            | Metrowerks Standard Library C++ headers |
| `include/compiler/`           | Compiler intrinsic headers |
| `include/PowerPC_EABI_Support/` | PowerPC EABI support headers |
| `include/Runtime.PPCEABI.H/`  | Runtime headers |
| `include/OdemuExi2/`          | Debug EXI headers |
| `include/types.h`             | Core type definitions (`u8`, `u16`, `u32`, `s8`, `s16`, `s32`, `f32`, `f64`, etc.) |
| `include/macros.h`            | Common macros |
| `include/ac_*.h`              | Actor headers (one per actor type) |
| `include/m_*.h`               | Game module headers |
| `include/sys_*.h`             | System utility headers |

---

## Key Architectural Concepts

### N64 Heritage via emu64

Animal Crossing was originally *Doubutsu no Mori* for the N64. The GameCube version retains the N64 rendering architecture — game code emits **N64 GBI (Graphics Binary Interface) display lists** (`Gfx` commands like `gSPVertex`, `gSP2Triangles`, `gDPSetCombineMode`, etc., defined in `include/PR/gbi.h`).

The **emu64** layer (`include/libforest/emu64/emu64.hpp`, `src/static/libforest/emu64/`) interprets these N64 display lists at runtime and translates them into **GameCube GX** API calls. This is the core graphics abstraction layer.

### Actor System

The game uses an actor system where each game entity (NPC, item, structure, effect, etc.) is an "actor" with standardized lifecycle callbacks (`init`, `move`/`update`, `draw`, `destroy`). Actors are registered in `m_actor_dlftbls.c` and managed by the actor system in `m_actor.c`.

### REL Module (foresta)

Most game code lives in a REL (relocatable ELF) module called **foresta**, which is loaded by the DOL at boot. The `executor.c` file provides the REL's prolog/epilog. This is how Nintendo structured many GameCube games to manage memory and loading.

### Thread Architecture

The game runs multiple OS threads (on the Dolphin OS threading model):
- **Main thread** (`mainproc` in `main.c`) — initialization and idle
- **Graph thread** (`graph_proc` in `graph.c`) — rendering pipeline
- **Pad manager thread** (`padmgr.c`) — controller polling
- **IRQ manager** (`irqmgr.c`) — interrupt handling

### NES Emulator

The game includes a built-in NES emulator (`src/static/Famicom/`, `include/Famicom/`) with its own audio subsystem (`jaudio_NES`), allowing players to play classic NES games on in-game furniture items.

---

## Conventions

- **C source** (`.c`) is used for nearly all game code. A small amount of **C++** (`.cpp`) is used for JSystem wrappers, the emu64 print module, and the NES emulator.
- **`.c_inc` files** are C source fragments `#include`d into their parent `.c` file to split large translation units (especially player logic and NPC behavior).
- **Prefix naming**: `ac_` = actor, `m_` = module/game system, `ef_` = effect, `bg_` = background item, `sys_` = system, `Na_`/`sAdo_` = audio.
- **N64 types** are used throughout: `Gfx` (display list command), `Mtx` (matrix), `Vtx` (vertex), `xyz_t` (3D vector), etc.
- The macro `VERSION` selects region-specific code paths (`VER_GAFE01_00` = 0, `VER_GAFU01_00` = 1).

---

## PC Build (CMake + clang-cl)

The project uses CMake for building. Theoretically should support any compiler, in practice, clang-cl is preferred on windows.

### Configure

```
cmake -G Ninja -B build_pc `
    -DCMAKE_C_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin/clang-cl.exe" `
    -DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin/clang-cl.exe" `
    -DCMAKE_LINKER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin/lld-link.exe" `
    -DCMAKE_BUILD_TYPE=Debug
```

To use MSVC's **link.exe** instead of lld-link (e.g. for reliable `animal_crossing.lib` import library generation), add `-DFOREST_USE_MSVC_LINKER=ON` and omit `-DCMAKE_LINKER=...`. The project will locate `link.exe` from your Visual Studio install. Alternatively, set `-DCMAKE_LINKER=".../VC/Tools/MSVC/<ver>/bin/Hostx86/x86/link.exe"` to a specific path.

This generates Ninja build files in `build_pc/`. A `compile_commands.json` is also produced for IDE/tooling support.

### Build

```
cmake --build build_pc
```

### Build Structure

The CMake build mirrors the original decomp structure:

| CMake Target | Type | Description |
|-------------|------|-------------|
| `forest_dol` | Executable | Static DOL — system code, SDK, engine (links all static libs) |
| `foresta_rel` | Shared Library (DLL) | REL module — all game logic (actors, scenes, events) |
| `boot`, `libforest`, `libu64`, `libc64`, `libultra_gu`, `libjsys` | Static Libraries | Engine/SDK support libraries (linked into DOL) |
| `JKernel`, `JSupport`, `JFramework`, `JUtility`, `J2D` | Static Libraries | JSystem libraries (linked into DOL) |
| `jaudio_NES_*`, `Famicom` | Static Libraries | NES emulator and audio (linked into DOL) |
| `dolphin_*` | Static Libraries | Dolphin SDK reimplementations (linked into DOL) |
| `foresta`, `game`, `actor`, `actor_npc`, `actor_npc_event`, `actor_tool` | Static Libraries | Game logic modules (linked into REL DLL) |
| `furniture`, `bg_item`, `effect`, `system`, `dataobject` | Static Libraries | Game content (linked into REL DLL) |
