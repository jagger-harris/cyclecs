#!/bin/sh

build=false
release=false
run=false
x11=false

# Parse args
for arg in "$@"; do
  case $arg in
    build) build=true ;;
    release) release=true ;;
    run) run=true ;;
    force-x11) x11=true ;;
  esac
done

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
      if $x11; then
        XDG_SESSION_TYPE=x11 ./executable
      else
        ./executable
      fi
    else
      cd debug || exit
      if $x11; then
        XDG_SESSION_TYPE=x11 ./executable debug 2> asan.log
      else
        ./executable debug 2> asan.log
      fi
    fi
  else
    echo "Build folder not located: ./build.sh build (release)"
  fi
fi

if ! $build && ! $run; then
  echo "Command usage: ./build.sh [build (release)] [run] [force-x11]"
fi
