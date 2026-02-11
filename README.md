# A-Novel-on-Underwater-Robot-Coordinate-Tracking-Algorithm-based-on-USBL
![Uploading image.png…]()

A Unity-based research project for **USBL (Ultra-Short Baseline) underwater robot coordinate tracking**.

> ⚠️ **Hardware requirement (critical):**  
> This project is designed to run **only when the X150 device is connected via RS232**.  
> If the device is not connected (or the serial port cannot be opened), the application may fail to start correctly or may not function as intended.

---

## Repository layout (important paths)

- **Executable (Windows build)**
  - `Program/Airo.exe`  
  - Keep `Airo.exe` together with the generated Unity runtime files in the same folder (e.g., `Airo_Data/`, `UnityPlayer.dll`, etc.).

- **Experimental data**
  - `UnderWater vehicle Tracking Data/`  
  - Contains experimental datasets (e.g., `.xlsx`, `.csv`) used in the research.

- **Core source code**
  - `ProjectFiles/src/`  
  - Contains the main C++ source files used for the tracking/control logic and RS232 serial communication.

- **Unity project folders**
  - `Assets/`, `Packages/`, `ProjectSettings/`

- **Native plugin reference**
  - `USBL_Unity_Cpp_Plugin/`  
  - Example/reference structure for building a Unity native C++ plugin (CMake-based).

---

## Requirements

### Hardware
<img width="364" height="364" alt="image" src="https://github.com/user-attachments/assets/09c07ffe-2ff2-4cbe-9a73-dad1c7101172" />

- **X150 device**
- RS232 connection to the PC  
  - If you are using a USB-to-RS232 adapter, make sure its driver is installed and the COM port appears in Windows Device Manager.

### Software
- **Unity Editor**: `2020.3.25f1` (see `ProjectSettings/ProjectVersion.txt`)
- Windows is assumed for running the provided `Airo.exe`.

---

## Serial communication settings (default)

The default serial configuration in `src/serial_port.hpp` is:

- Baud rate: **115200**
- Data bits: **8**
- Parity: **None**
- Stop bits: **2** (8N2)

> If the X150 device is configured differently, update the application settings and/or the code to match the device configuration.

---

## Running the prebuilt executable (Windows)

1. Connect the **X150** device to the PC via **RS232**.
2. Check the COM port number in **Device Manager** (e.g., `COM3`, `COM10`).
3. Run:
   - `Program/Airo.exe`
4. In the application UI (or configuration, depending on your build), select the correct COM port and confirm the baud rate (`115200` by default).

**Note (COM ports >= 10):**  
On Windows, ports like `COM10` sometimes require the `\\.\COM10` format internally.  
This project’s serial wrapper includes logic to handle this case.

---

## Opening in Unity (development)

1. Open the project root in **Unity 2020.3.25f1**.
2. Enter Play Mode to test the Unity scene logic.

> ⚠️ Without a connected X150 (RS232), anything that depends on real-time serial input will not operate correctly.

---

## Core code (where to look)

Key files in `src/` include:

- `serial_port.hpp / serial_port.cpp`  
  Cross-platform serial port wrapper (Windows WinAPI / POSIX termios).

- `usbl_control.hpp`  
  USBL-based tracking/control components (e.g., path/heading control).

- `surface_station_main.cpp`, `vehicle_node_main.cpp`  
  Main entry points used for “surface station” / “vehicle node” style runs (research structure).

- `seatrac.hpp.cpp`  
  Protocol/telemetry handling implementation (project-specific).

---
