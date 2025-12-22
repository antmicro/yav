# Yet Another Viewer

Copyright (c) 2025 [Antmicro](https://www.antmicro.com)

Show images using the Linux framebuffer or DRM at specific locations on the screen.

## Build

To build YAV with the optional Linux DRM support, first, install `libdrm-dev`:

```bash
# Debian
apt install libdrm-dev
```

YAV uses CMake and can be build using standard CMake commands like so:

```bash
cmake -Bbuild .
cmake --build build
```

## Usage

YAV will not work together with a running Linux desktop environment, to use it run it without one or on a separate TTY.
To learn more, see `yav --help`.

```bash
./build/yav --image example/tuxan.png --anchor 0.5 0.5
```

## License

This project is licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE).
