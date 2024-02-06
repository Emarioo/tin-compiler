@echo off
@setlocal enabledelayedexpansion

@REM Toggle comments on the variables below to change compiler
@REM SET USE_MSVC=1
SET USE_GCC=1

SET SRC= AST.cpp Code.cpp Compiler.cpp Generator.cpp Interpreter.cpp Lexer.cpp main.cpp Parser.cpp Util.cpp
SET SRC=!SRC: = src/!

mkdir bin 2> nul

if !USE_MSVC!==1 (
    SET DEFS=/DOS_WINDOWS

    cl /nologo /Zi !SRC! !DEFS! /EHsc /I include /FI pch.h /Fobin\ /link /DEBUG /OUT:app.exe
    SET error=!errorlevel!
) else if !USE_GCC!==1 (
    SET DEFS=-DOS_WINDOWS

    g++ -g !SRC! !DEFS! -Iinclude -include include/pch.h -o app.exe
    SET error=!errorlevel!
)

if !error!==0 (
    app
)
