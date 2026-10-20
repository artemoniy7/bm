# Apex Rush — C++ 3D Racing

A C++17 3D arcade racing prototype built with raylib. It includes a main menu, three layouts (two circuits and a straight drag strip), procedural sports cars and scenery, a chase camera, AI rivals, free-body driving, collisions, speed HUD, lap timing, and a pause/results screen.

## Build

Requirements: CMake 3.16+, a C++17 compiler, and internet access on the first configure (CMake downloads raylib).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/apex_rush
```

On Windows, run `build\\Release\\apex_rush.exe` if using a multi-configuration generator.

## Controls

- **W** — accelerate
- **S** — brake, then reverse
- **A / D** — steer left / right
- **Space** — handbrake / drift
- **Esc** — pause
- **Enter** — confirm menu selection
- **A / D** in the menu — select a track

## Tracks

- **Coastal Run** — wide circuit with greenery.
- **Alpine Pass** — alternate circuit layout and palette.
- **Drag Strip** — long straight acceleration course with a finish line.

## Driving and collisions

The car moves through world-space coordinates instead of being snapped to a track lane. Steering depends on speed and direction; lateral slip creates a simple drift model. Driving off the asphalt remains possible but reduces speed and grip. Player/AI contact pushes cars apart and reduces speed; roadside obstacles also cause impacts. Cars and scenery are assembled from 3D primitives, so external art assets are not required.

This is an arcade prototype rather than a full tire, suspension, or damage simulation.