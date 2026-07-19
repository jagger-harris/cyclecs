# Cyclecs Engine
A custom game engine written in C (C17) which uses a custom ECS architecture for game objects and logic. Currently only tested on Linux.

## Libraries
- [CGLM](https://github.com/recp/cglm) - 0.9.6 - MIT
- [FreeType](https://gitlab.freedesktop.org/freetype/freetype) - 2.14.1 - Custom
- [GLFW](https://github.com/glfw/glfw) - 3.4 - Zlib
- [Glad 2](https://github.com/Dav1dde/glad) - OpenGL 3.3 Core - MIT
- [stb_image](https://github.com/nothings/stb) - 2.30 - MIT
- [stb_image_write](https://github.com/nothings/stb) - 1.16 - MIT

## Dependencies
- OpenGL >= 3.3 supported graphics
- CMake >= 3.21
- Wayland or Xorg
- Clang or GCC C17 compiler

## Getting and Running
Clone the repository:
```shell
git clone https://github.com/jagger-harris/cyclecs.git
cd cyclecs
```

Debug build:
```shell
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=debug
cmake --build build/debug
./build/debug/executable
```

Release build:
```shell
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=release
cmake --build build/release
./build/release/executable
```

## Building Documentation
For building documentation, you can read how to [here](docs/README.md).

## Contributing
This is a personal project. Contributions will not be accepted as of now.

## License
- [LGPL 3.0](https://choosealicense.com/licenses/lgpl-3.0/)
