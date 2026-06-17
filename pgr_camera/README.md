# pgr_camera

Hello-world for a FLIR/Point Grey camera via the Spinnaker SDK.

Detects the first connected camera, prints its model/serial/firmware, grabs
one frame, and writes it to `frame.pgm`.

Tested against a **Chameleon3 CM3-U3-50S5C** on USB3.

## Prerequisites

- Spinnaker SDK installed (default location: `/opt/spinnaker/`). Get it from
  https://www.flir.com/products/spinnaker-sdk/ — requires a free FLIR account.
  Run the bundled `sudo sh install_spinnaker.sh` and accept the EULA.
- Current user in the `flirimaging` group:
  ```
  sudo usermod -a -G flirimaging $USER
  # log out / log back in to pick up the group
  ```
- CMake 3.16+, a recent g++.

## Build & run

```
mkdir build && cd build
cmake ..
cmake --build .
./hello_world
```

If Spinnaker isn't at `/opt/spinnaker`:
```
cmake -DSPINNAKER_ROOT=/your/path ..
```

## Expected output

```
Spinnaker 3.x.y.z
Cameras detected: 1
  DeviceModelName: Chameleon3 CM3-U3-50S5C
  DeviceSerialNumber: 12345678
  DeviceVersion: ...
Captured 2448x2048  format=BayerRG8
Wrote frame.pgm
```

`frame.pgm` is the raw Bayer mosaic written as 8-bit grayscale — viewable in
any image tool but not yet demosaiced to color. That's the next step.

## Troubleshooting

- **"Cameras detected: 0"** — check `lsusb | grep -i 'Point Grey'`. If the
  camera is visible but not detected by Spinnaker, you're probably not in the
  `flirimaging` group yet.
- **"Image incomplete"** — usually a USB bandwidth / power issue. Try a
  different USB3 port (a real one on the motherboard, not through a hub).
- **`libSpinnaker.so` not found at runtime** — CMake embeds the rpath, but
  if you moved the binary, set `LD_LIBRARY_PATH=/opt/spinnaker/lib`.
