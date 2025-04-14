# Jotner

## Description

This is a small lightweight magnifying/zooming application for [Linux](https://en.wikipedia.org/wiki/Linux) written in the 
[C programming language](https://en.wikipedia.org/wiki/C_(programming_language)).
This is HEAVILY inspired by [boomer](https://github.com/tsoding/boomer), but is made to be faster, smaller and has no issues with WM animations 

## Naming

Named after [Jötnar (Frost Giants)](https://en.wikipedia.org/wiki/J%C3%B6tunn) from Norse mythology because I am bad at naming

## Dependencies

- X11
- OpenGL

## Installation

## Download Release

Use `wget` to download or download from the release tab

```bash
wget https://github.com/Agent11196/jotner/releases/download/v1.0/jotner-x86_64-Linux-v1.0.tar.gz
```

Use `tar` to extract
```bash
tar -xzf jotner-x86_64-Linux-v1.0.tar.gz
```

Run
```bash
./jotner
```

## Or Build From Source

### Build System Needed:
- make

### 1. Clone the repository
```bash
git clone https://github.com/Agent11196/jotner
```

### 2. Use `make` to compile the program
```bash
make release
```

### 2.1. In case for debug builds simply use `make`
```bash
make
```

### 3. Run
```bash
./build/rel/jotner
```

### 3.1. Run debug build
```bash
./build/dbg/jotner
```

## Controls
| Control | Action |
|---------|--------|
|Scroll up| Zoom in |
|Scroll down| Zoom out |
|Mouse Click + Drag| Pan the camera |

## Configuration
To keep it as fast as possible there are no `config` files but configuration is done via arguments.
| Argument | Action |
|----------|--------|
|-h, --help | Shows the list of arguments and the usage |
| -v, --version| show the version of program and exit |
| -f, --frames-per-second <int32=200> | the frame rate of the app (recommended 2x the frame rate of monitor) |
| -z, --zoom-sensitivity <float32=0.1> | scroll sensitivity for zooming|
| -p, --panning-sensitivity <float32=1.0> | mouse cursor sensitivity for panning the camera |
