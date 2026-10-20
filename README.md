# Apex Rush — C++ 3D Racing

A small 3D arcade racing game built with C++ and raylib. It features a main menu, three tracks, procedural 3D cars and scenery, arcade driving physics, AI opponents, checkpoints, lap timing, and a pause menu.

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
- **Space** — handbrake
- **Esc** — pause / back to menu
- **Enter** — confirm menu selection

Tracks: Coastal Run, Alpine Pass, and Neon Circuit. Models and track scenery are procedurally built from 3D primitives, so no external assets are required.

## Notes

This is an arcade-style prototype rather than a full simulation. Raylib is fetched automatically by CMake.