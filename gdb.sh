#!/bin/bash

GAME_DIR="$(pwd)/game"

#export LD_LIBRARY_PATH="$GAME_DIR/bin/linux64:$GAME_DIR/mod_tf/bin/linux64":$LD_LIBRARY_PATH
export GAME_DEBUGGER="cgdb"

./game/mod_tf_linux64 -novid -console -windowed -w 1920 -h 1080 +sv_lan 1 +sv_cheats 1 +developer 1 +log on