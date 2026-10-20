# Apex Rush — C++ 3D Racing

A C++17 3D arcade racing prototype built with raylib, featuring upgraded procedural sports-car models, track curbs, improved steering response, speed-sensitive handling, and a handbrake drift mode. It features a main menu, three tracks, procedural 3D cars and scenery, arcade driving physics, AI opponents, checkpoints, lap timing, and a pause menu.

## Build

Requirements: CMake 3.16+, a C++17 compiler, and internet access on the first configure (CMake downloads raylib).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/apex_rush
```

On Windows, run `build\\Release\\apex_rush.exe` if using a multi-configuration generator.

## Controls

- **W / Up** — accelerate
- **S / Down** — brake / reverse
- **A / D** or **Left / Right** — steer
- **Space** — handbrake / drift
- **Esc** — pause / back to menu
- **Enter** — confirm menu selection

Tracks: Coastal Run, Alpine Pass, and Neon Circuit. Models and track scenery are procedurally built from 3D primitives, so no external assets are required.

## Notes

Driving uses an arcade model with acceleration, braking, drag, speed-sensitive steering, lateral line control, and handbrake grip changes. Cars are assembled procedurally from 3D primitives, with no external assets required. This is still a prototype, not a full tire/suspension simulation. Raylib is fetched automatically by CMake.