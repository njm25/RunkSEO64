# Runkseo64

An updated version of **Seo64**, an internal project for 64-bit Team Fortress 2.

It builds into a DLL (`Runkseo64.dll`) that runs inside the TF2 process. It hooks the Source engine and draws an ImGui overlay on top of the game's D3D9 renderer.

## Features

- **Aimbot**: separate hitscan, projectile, and melee modes, plus shared aim utilities
- **Prediction**: engine prediction, projectile simulation, strafe prediction, seed prediction, and tick-base handling
- **Crits**: crit handling and tracking
- **Visuals**: entity ESP, hitbox drawing, material modulation, class icons, and a minimap
- **Automation**: auto backstab, auto airblast/reflect, auto detonate, auto Vaccinator, and auto queue
- **Movement and anti-aim**: game-movement helpers and anti-aim
- **Misc**: player list, spectator list, notifications, party commands, networking tweaks, and animation fixes
- **UI**: ImGui menu with keybinds and JSON configs

## Usage

| Key | Action |
| --- | --- |
| `Insert` | Toggle the menu |
| `F11` | Unload |

Configs are saved to `%APPDATA%\Runkseo64\`.

If signatures, interfaces, or cvars fail to resolve when loading, a message box lists the failures, which usually means a game update broke something.

## Building

Requirements:
- Visual Studio 2022 (toolset v143)
- Windows SDK
- C++20

Open `solution.sln`, select `Release | x64`, and build. Output goes to `output\bin\<Configuration>\Runkseo64.dll`, and intermediates go to `output\build\`. The `output\` folder is gitignored.

## Project layout

```
project/src/
├── main.cpp          entry point: load/unload, error reporting
├── app/
│   ├── hooks.cpp     engine hooks
│   └── features/     every feature, one folder each (see above)
├── game/             TF2 game classes and helpers
├── sdk/              Source SDK definitions
└── shared/
    ├── utils/        shared utilities
    └── libs/         third-party libraries
```

## Third-party libraries

- [Dear ImGui](https://github.com/ocornut/imgui)
- [MinHook](https://github.com/TsudaKageyu/minhook)
- [FreeType](https://freetype.org/) (prebuilt `freetype.lib` included)
- [nlohmann/json](https://github.com/nlohmann/json)
