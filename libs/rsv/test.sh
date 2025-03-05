#!/bin/sh

build=false
test=false

if ! [ -d "./build" ]; then
  mkdir build
fi

cd build || exit
cmake ..

if [ -e "compile_commands.json" ]; then
  mv ./compile_commands.json ../compile_commands.json
fi

make
cd .. || exit

if [ -d "./build" ]; then
  cd build || exit
  ctest --verbose --rerun-failed --output-on-failure
else
  echo "Build folder not located..."
fi
