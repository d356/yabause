set MSYSTEM=MINGW64
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\msys64;%PATH%
C:\msys64\usr\bin\bash -lc "pacman --noconfirm -Sy"
C:\msys64\usr\bin\bash -lc "pacman --noconfirm -S mingw64/mingw-w64-x86_64-SDL2 mingw-w64-x86_64-binutils mingw64/mingw-w64-x86_64-qt5 mingw-w64-x86_64-crt-git mingw-w64-x86_64-headers-git mingw64/mingw-w64-x86_64-libwebp"
C:\msys64\usr\bin\bash -lc "cd C:/projects/yabause/yabause/build ; cmake -G\"MSYS Makefiles\" -DCMAKE_BUILD_TYPE=Release -DDirectX_INCLUDE_DIR=C:/msys64/mingw64/x86_64-w64-mingw32/include -DYAB_WANT_DIRECTSOUND=ON -DYAB_WANT_DIRECTINPUT=ON -DYAB_NETWORK=ON -DYAB_WANT_GDBSTUB=ON -DMSYS2_BUILD=ON -DCMAKE_PREFIX_PATH=\"C:/msys64/mingw64/bin;C:/msys64/mingw64/include;C:/msys64/mingw64/lib;C:/msys64/mingw64/x86_64-w64-mingw32/include;C:/msys64/mingw64/x86_64-w64-mingw32/lib\" -DSDL2_LIBRARY=C:/msys64/mingw64/lib/libSDL2.dll.a .."
