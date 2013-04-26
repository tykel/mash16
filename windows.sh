## Major hack...

MAJOR=0
MINOR=5
REV=8

echo "building mash16.exe ..."
i486-mingw32-gcc -o build/mash16.exe -O2 -std=c99 -Wall -DMAJOR=$MAJOR -DMINOR=$MINOR -DREV=$REV -DBUILD=\"`git rev-parse HEAD | cut -c-10`\" -I/opt/SDL-1.2.13/include -D_GNU_SOURCE=1 -Dmain=SDL_main -D_REENTRANT -L/opt/SDL-1.2.13/lib src/header/*.c src/core/*.c src/*.c -lmingw32 -lSDLmain -lSDL -mwindows
i486-mingw32-strip build/mash16.exe

echo "zipping relevant files ..."
zip archive/mash16_$MAJOR.$MINOR.$REV.zip -j build/mash16.exe README SPEC.1.1 ~/.wine/drive_c/windows/syswow64/SDL.dll

echo "done."
