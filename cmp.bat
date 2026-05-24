@echo off
setlocal enabledelayedexpansion
set ERROR_FOUND=0
cls
echo == Cngine v2 Build ==
echo.

cd "engine"

REM ================================================================
REM  LUA SOURCE FILES
REM ================================================================
echo [1/4] Compiling Lua 5.4...

gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lapi.c     -o lapi_lua.o     2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lauxlib.c  -o lauxlib_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lbaselib.c -o lbaselib_lua.o 2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lcode.c    -o lcode_lua.o    2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lcorolib.c -o lcorolib_lua.o 2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lctype.c   -o lctype_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ldblib.c   -o ldblib_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ldebug.c   -o ldebug_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ldo.c      -o ldo_lua.o      2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ldump.c    -o ldump_lua.o    2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lfunc.c    -o lfunc_lua.o    2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lgc.c      -o lgc_lua.o      2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\linit.c    -o linit_lua.o    2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\liolib.c   -o liolib_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\llex.c     -o llex_lua.o     2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lmathlib.c -o lmathlib_lua.o 2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lmem.c     -o lmem_lua.o     2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\loadlib.c  -o loadlib_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lobject.c  -o lobject_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lopcodes.c -o lopcodes_lua.o 2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\loslib.c   -o loslib_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lparser.c  -o lparser_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lstate.c   -o lstate_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lstring.c  -o lstring_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lstrlib.c  -o lstrlib_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ltable.c   -o ltable_lua.o   2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ltablib.c  -o ltablib_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\ltm.c      -o ltm_lua.o      2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lundump.c  -o lundump_lua.o  2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lutf8lib.c -o lutf8lib_lua.o 2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lvm.c      -o lvm_lua.o      2>&1 && ^
gcc -m64 -I. -Ilua -DLUA_USE_WINDOWS -c lua\lzio.c     -o lzio_lua.o     2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

REM ================================================================
REM  ENGINE MODULES
REM ================================================================
echo [2/4] Compiling engine modules...

echo   engine_math...
gcc -m64 -I. -Ilua -c engine_math.c   -o engine_math.o   2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   physik...
gcc -m64 -I. -Ilua -c physik.c        -o physik.o        2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   render...
gcc -m64 -I. -Ilua -c render.c        -o render.o        2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   core...
gcc -m64 -I. -Ilua -c core.c          -o core.o          2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   hitboxes...
gcc -m64 -I. -Ilua -c hitboxes.c      -o hitboxes.o      2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   audio...
gcc -m64 -I. -Ilua -Isdl -c audio.c         -o audio.o         2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   postfx_config...
gcc -m64 -I. -Ilua -c postfx_config.c -o postfx_config.o 2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo   scripts...
gcc -m64 -I. -Ilua -Isdl -DLUA_USE_WINDOWS -c scripts.c -o scripts.o 2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

REM ================================================================
REM  GAME
REM ================================================================
echo [3/4] Compiling game...
gcc -m64 -I. -Ilua -Isdl -DLUA_USE_WINDOWS -c ..\game.c -o game.o 2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

REM ================================================================
REM  LINK
REM ================================================================
echo [4/4] Linking...
gcc -m64 -o ..\game.exe ^
    engine_math.o physik.o render.o core.o ^
    hitboxes.o audio.o postfx_config.o scripts.o game.o ^
    lapi_lua.o lauxlib_lua.o lbaselib_lua.o lcode_lua.o lcorolib_lua.o ^
    lctype_lua.o ldblib_lua.o ldebug_lua.o ldo_lua.o ldump_lua.o ^
    lfunc_lua.o lgc_lua.o linit_lua.o liolib_lua.o llex_lua.o ^
    lmathlib_lua.o lmem_lua.o loadlib_lua.o lobject_lua.o lopcodes_lua.o ^
    loslib_lua.o lparser_lua.o lstate_lua.o lstring_lua.o lstrlib_lua.o ^
    ltable_lua.o ltablib_lua.o ltm_lua.o lundump_lua.o lutf8lib_lua.o ^
    lvm_lua.o lzio_lua.o ^
    -L.\sdl\lib -lmingw32 -lSDL2main -lSDL2 -lm -lwinmm 2>&1
if !errorlevel! neq 0 set ERROR_FOUND=1

echo.
if %ERROR_FOUND% equ 1 (
    echo ==============================
    echo  BUILD FAILED
    echo ==============================
) else (
    echo ==============================
    echo  BUILD OK  -  run game.exe
    echo  SDL2.dll must be next to game.exe
    echo  Lua scripts: scripts\ folder
    echo ==============================
)
echo.
pause
