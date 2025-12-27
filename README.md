# SkyrimSE-FPInertia

Adds first person inertia to Skyrim Special Edition.

## Requirements

### Runtime
- Skyrim Special Edition (1.6.x+)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

### Building from Source
- **Visual Studio 2022** with "Desktop development with C++" workload
- **CMake 3.21+** (add to PATH)
- **vcpkg** (set `VCPKG_ROOT` environment variable to vcpkg install path)
- **MSVC Toolset** v14.43+ (included with VS2022)

## Building

1. Clone the repository:
   ```
   git clone https://github.com/DCCStudios/SkyrimSE-FPInertia.git
   cd SkyrimSE-FPInertia
   ```

2. Configure with CMake:
   ```
   cmake --preset release
   ```

3. Build:
   ```
   cmake --build build/release --config Release
   ```

4. The compiled plugin will be in `Compile/SKSE/Plugins/`.

## Configuration

Edit `Data/SKSE/Plugins/FPInertia.ini` to customize settings.

## License

MIT License
