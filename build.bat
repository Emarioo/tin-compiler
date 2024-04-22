@echo off
@setlocal enabledelayedexpansion

@REM make -j 8
@REM if !errorlevel!==0 (
@REM     bin\app
@REM )

@REM ##################
@REM      NO MAKE
@REM #################

@REM Toggle comments on the variables below to change compiler
SET USE_MSVC=1
@REM SET USE_GCC=1

SET SRC= AST.cpp Bytecode.cpp Compiler.cpp Generator.cpp Interpreter.cpp Lexer.cpp main.cpp Parser.cpp TinGenerator.cpp Util.cpp
SET SRC=!SRC: = src/!

SET SRC=!SRC! libs/tracy-0.10/public/TracyClient.cpp

SET UNITY_FILE=bin/all.cpp
type nul > !UNITY_FILE!

for /r %%i in (*.cpp) do (
    SET file=%%i
    if "x!file:__=!"=="x!file!" if "x!file:bin=!"=="x!file!" if not "x!file:src=!"=="x!file!" (
        echo #include ^"!file:\=/!^">> !UNITY_FILE!
    )
)
@REM echo #include "D:/Backup/CodeProjects/exjobb-compiler/libs/tracy-0.10/public/TracyClient.cpp" >> !UNITY_FILE!

SET SRC=!UNITY_FILE!

mkdir bin 2> nul

if !USE_MSVC!==1 (
    SET DEFS=/DOS_WINDOWS
    SET DEFS=!DEFS! /DTRACY_ENABLE

    if not exist bin\TracyClient.obj (
        cl /nologo /c /Zi /std:c++14 /TP /DTRACY_ENABLE libs/tracy-0.10/public/TracyClient.cpp /EHsc /I libs/tracy-0.10/public /Fobin\
    )
    
    cl /nologo /Zi !SRC! !DEFS! /EHsc /std:c++14 /TP /I . /I include /I libs/tracy-0.10/public /FI pch.h /Fobin\ /link /DEBUG bin\TracyClient.obj /OUT:bin\app.exe
    
    SET error=!errorlevel!

) else if !USE_GCC!==1 (
    SET DEFS=-DOS_WINDOWS

    g++ -g !SRC! !DEFS! -Iinclude -include include/pch.h -o app.exe
    SET error=!errorlevel!
)

if !errorlevel!==0 (
    bin\app
    @REM echo f | xcopy bin\app.exe app.exe /y /q > nul
)
