#!/bin/sh

build=false
release=false
run=false

if [ "$1" = "build" ]; then
  build=true;

  if [ "$2" = "release" ]; then
    release=true;
  fi
fi

if [ "$1" = "run" ] || [ "$2" = "run" ] || [ "$3" = "run" ]; then
  run=true;
fi

if $build; then
  if $release; then
    cmake -S . -B build/release -DCMAKE_BUILD_TYPE=release
    cmake --build build/release
  else
    cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=debug
    cmake --build build/debug
    
    if [ -e "build/debug/compile_commands.json" ]; then
      mv build/debug/compile_commands.json .
    fi
  fi
fi

if $run; then
  if [ -d "build" ]; then
    cd build || exit
    
    if $release; then
      cd release || exit
      ./executable
    else
      cd debug || exit
      ./executable debug 2> asan.log
    fi
  else
    echo "Build folder not located: ./build.sh build (release)"
  fi
fi

if ! $build && ! $run; then
  echo "Command usage: ./build.sh command [command: build (release), run, build (release) run]"
fi
