# BorealOS

BorealOS is a C++ based operating system with Limine as a bootloader.

## Building
To build BorealOS, initialize the git submodules and run the build script:

```bash
git submodule update --init --recursive
./build.sh
```

The resulting ISO image will be located in the `bin/` directory.

## Running
You can run BorealOS using QEMU with the following command:
```bash
./run.sh
```
For debug mode in QEMU, use:
```bash
./run.sh --debug
```