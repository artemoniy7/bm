# FLEX DRIVE — C++ Vehicle Physics Sandbox

A clean-slate 3D driving and crash-test prototype in C++17 + raylib.

## Features
- Main menu, pause menu, and resettable test session.
- Detailed procedural road-car model: body panels, glass, wheels, rims, lights, and visible chassis frame.
- Spring-based deformable chassis using Verlet integration and iterative distance constraints.
- Collision response, crumple-zone spring weakening, structural damage indicator, vehicle condition bar, and flying debris.
- Test pad with barriers, concrete blocks, ramps, pylons, and open driving space.
- Follow camera, speed readout, driving controls, and togglable help overlay.

> This is an original prototype inspired by vehicle-physics sandbox games, not a full BeamNG.drive-equivalent soft-body solver. Cars are procedural placeholder models, not licensed real-world vehicle meshes. Damage simulation is an initial spring-frame approximation.

## Build
Requires CMake 3.20+, a C++17 compiler, and internet access on first configure to fetch raylib.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/flex_drive
```

Windows multi-configuration builds usually place the executable at `build/Release/flex_drive.exe`.

## Controls
- W / Up: accelerate
- S / Down: brake / reverse
- A / D or Left / Right: steer
- R: reset car
- Tab: toggle driving help
- Esc: pause/resume
- Enter: start / resume
- M: return to main menu (paused)
