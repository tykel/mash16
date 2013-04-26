# Mash16 -- Advanced Chip16 Emulator

## Summary

Mash16 is an emulator for the Chip16 platform, which aims to make writing
emulators easier for beginners.

It supports the full instruction set, and the user is encouraged to read the
source code.

## Features

- Full support of the latest (1.2) Chip16 specification
- Debugging functionality (breakpoints, stepping, state viewing)
- Multiple video scalers
- Unrestricted emulation speed possible

## Usage
    mash16 filename [OPTIONS]

## Options
    --no-audio              : disable audio output
    --audio-sample-rate=n   : set audio sample rate (8000,11025,22050,44100 recommended)
    --audio-buffer=n        : set audio buffer size (512-8192 recommended)
    --verbose               : log emulation
    --video-scaler=n        : screen scaler size (1,2,3,4)
    --fullscreen            : use fullscreen mode
    --no-cpu-limit          : do not restrict the speed of the emulation

    --cpu-rec               : use (experimental) recompiler core

    --break@n               : add a breakpoint at address n
