
# USBL Unity C++ Native Plugin (Reference Implementation)

This project is a **Unity-native-plugin-friendly C++ implementation** of the USBL-based
underwater robot coordinate tracking & steering-control workflow described in the attached manuscript.

> Important: the beacon command/telemetry wire format is **device-specific**.
> The paper explains the *algorithm*, not the serial protocol.
> Therefore, this repository provides:
> - A tolerant `key=value` telemetry parser (easy to adapt)
> - A safe template command string builder (easy to replace)

## 1) Build the plugin

### Windows (Visual Studio / MSVC)
```bash
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

### Linux / macOS
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Outputs:
- Windows: `build/Release/USBLUnityPlugin.dll`
- Linux: `build/libUSBLUnityPlugin.so`
- macOS: `build/libUSBLUnityPlugin.dylib`

## 2) Unity integration

1. Create folder:
   - `Assets/Plugins/x86_64/`

2. Copy the built library into that folder.

3. Copy the C# scripts from `Unity/` into:
   - `Assets/Scripts/`

4. Add `USBLControllerExample` to a GameObject and configure:
   - Serial port (e.g., `COM3`)
   - Baud rate (e.g., `115200`)
   - Slave ID

## 3) Telemetry line format (example)

One line per update, delimited by `\n`:
```
ID=1,AZ=12.3,EL=5.0,DST=14.2,YAW=180,ROLL=0.1,PITCH=-0.2,PHX=0.12,PHY=-0.05,FREQ=25000,C=1500,DX=0.05,DY=0.05,TOF=0.0093
```

Supported keys (case-insensitive):
- `ID`
- `AZ|AZIMUTH`, `EL|ELEVATION`, `DST|DISTANCE|RANGE`
- `YAW`, `ROLL`, `PITCH`
- `PHX`, `PHY`, `FREQ`, `C` (sound speed), `DX`, `DY`, `TOF`

If your X150 outputs a different format, modify `src/parser.cpp`.

## 4) Default command format

The plugin can send a template command line:
```
$CMD,ID=<id>,STEER_DEG=<deg>,THR=<0..1>\r\n
```
You **must** replace this with your hardware's real command format.
See `Context::SendDefaultCommand()` in `src/usbl_plugin.cpp`.

## 5) Coordinate conventions

Internal tracking is in **ENU**:
- +X = East, +Y = North, +Z = Up

Unity is typically:
- +X = Right, +Y = Up, +Z = Forward

So, if you want North to be Unity-forward:
- Unity.x = ENU.x (East)
- Unity.y = ENU.z (Up)
- Unity.z = ENU.y (North)

The included C# example applies this mapping.

---
This is a reference codebase for integrating the algorithm into a Unity project.
