#!/bin/bash

# g++ 빌드 명령
g++ main.cpp chat_screen.cpp -o build/output.exe \
-I/mingw64/include/ncursesw \
-I/c/Projects/enet-1.3.18/include \
-L/mingw64/lib \
-L/c/Projects/enet-1.3.18/build \
-lncursesw -lpanelw -lenet -lws2_32 -lwinmm -pthread


# 빌드 결과 출력
if [ $? -eq 0 ]; then
    echo "Build succeeded! Output: build/output.exe"
else
    echo "Build failed!"
    exit 1
fi
