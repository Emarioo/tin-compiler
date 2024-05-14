@echo off
@setlocal enabledelayedexpansion

@REM make -j 8
@REM if !errorlevel!==0 (
@REM     bin\app
@REM )

@REM ##################
@REM      NO MAKE
@REM #################


SET arg=%1
if !arg!==run (
    @REM Run compiler with compiling it
    @REM btb -dev

    goto RUN_COMPILER

    @REM exit /b
)

@REM Toggle comments on the variables below to change compiler
SET USE_MSVC=1
@REM SET USE_OPTIMIZED=1
SET USE_TRACY=1
SET USE_DEBUG=1
@REM SET USE_GCC=1

@REM SET SRC= AST.cpp Bytecode.cpp Compiler.cpp Generator.cpp VirtualMachine.cpp Lexer.cpp main.cpp Parser.cpp TinGenerator.cpp Util.cpp
@REM SET SRC=!SRC: = src/!
@REM SET SRC=!SRC! libs/tracy-0.10/public/TracyClient.cpp

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

    SET OPTIONS=/EHsc /std:c++17 /TP
    SET OPTIONS_LINK=
    if !USE_OPTIMIZED!==1 (
        SET OPTIONS=!OPTIONS! /O2
    )
    if !USE_DEBUG!==1 (
        SET OPTIONS=!OPTIONS! /Zi
        SET OPTIONS_LINK=!OPTIONS_LINK! /DEBUG
    )
    if !USE_TRACY!==1 (
        SET DEFS=!DEFS! /DTRACY_ENABLE
        if not exist bin\TracyClient.obj (
            cl /nologo /c !OPTIONS! /DTRACY_ENABLE libs/tracy-0.10/public/TracyClient.cpp /I libs/tracy-0.10/public /Fobin\
        )
        SET OPTIONS_LINK=!OPTIONS_LINK! bin\TracyClient.obj
    )
    @REM RegGetValue in Util.cpp requires Advapi32
    cl /nologo !SRC! !DEFS! !OPTIONS! /I . /I include /I libs/tracy-0.10/public /FI pch.h /Fobin\ /link !OPTIONS_LINK! Advapi32.lib /OUT:bin\tin.exe
    
    SET error=!errorlevel!

) else if !USE_GCC!==1 (
    @REM BROKEN DISABLED, USE MAKEFILE instead
    @REM SET DEFS=-DOS_WINDOWS

    @REM g++ -g !SRC! !DEFS! -Iinclude -include include/pch.h -o bin/tin.exe
    @REM SET error=!errorlevel!
)

if !errorlevel!==0 (
    @REM echo f | xcopy bin\app.exe app.exe /y /q > nul
:RUN_COMPILER
    @REM bin\tin -dev -t 1
    bin\tin test.tin -t 1 -run
    @REM bin\tin -dev -t 16

    @REM SET OPTS=generated/main.tin -silent -measure -t
    @REM @REM @REM create file
    @REM SET OUT=out
    @REM type nul > !OUT!
    @REM @REM measure
    @REM bin\tin !OPTS! 2  >> !OUT!
    @REM bin\tin !OPTS! 4  >> !OUT!
    @REM bin\tin !OPTS! 6  >> !OUT!
    @REM bin\tin !OPTS! 8  >> !OUT!
    @REM bin\tin !OPTS! 10 >> !OUT!
    @REM bin\tin !OPTS! 12 >> !OUT!
    @REM bin\tin !OPTS! 14 >> !OUT!
    @REM bin\tin !OPTS! 16 >> !OUT!
)
