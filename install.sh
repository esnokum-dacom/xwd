#!/bin/bash

if [[ $1 ]]; then
    cmake --install build --prefix $1
    mkdir -p $HOME/.config/xwd
    touch $HOME/.config/xwd/config
    echo "Installed in ($1)"
else
    sudo cmake --install build
    mkdir -p $HOME/.config/xwd
    touch $HOME/.config/xwd/config
    echo "Installed in default location (/usr/local)"
fi
