# Cyclecs Engine
A custom game engine written in C (C17) which uses a custom ECS architecture for game objects. Currently only working for Linux as of now.

## Libraries
- CGLM - 0.9.6
- FreeType - 2.14.1
- GLFW - 3.4
- Glad - OpenGL 3.3 Core
- stb_image - 2.30
- stb_image_write - 1.16

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
mkdir build
cd build
mkdir debug
cd debug
cmake ../.. -DCMAKE_BUILD_TYPE=debug
make
./executable
```

Release build:
```shell
mkdir build
cd build
mkdir release
cd release
cmake ../.. -DCMAKE_BUILD_TYPE=release
make
./executable
```

## Contributing
This is a personal project. Contributions will not be accepted as of now.

## License
- [LGPL 3.0](https://choosealicense.com/licenses/lgpl-3.0/)
