#!/bin/bash

# You might have to modify this

# For windows, Visual studio puts it in a subfolder
if [ -f ./build/Release/main.exe ]; then
    ./build/Release/main.exe $@
fi

# For linux (and hopefully mac)
if [ -f ./build/main ]; then
    ./build/main $@
fi
