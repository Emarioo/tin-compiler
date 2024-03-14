@echo off
@setlocal enabledelayedexpansion

make -j 8
if !errorlevel!==0 (
    app
)

@REM ##################
@REM      NO MAKE
@REM #################

@REM Toggle comments on the variables below to change compiler
@REM SET USE_MSVC=1
@REM SET USE_GCC=1

@REM SET SRC= AST.cpp Code.cpp Compiler.cpp Generator.cpp Interpreter.cpp Lexer.cpp main.cpp Parser.cpp TinGenerator.cpp Util.cpp
@REM SET SRC=!SRC: = src/!

@REM SET SRC=!SRC! libs/tracy-0.10/public/TracyClient.cpp

@REM mkdir bin 2> nul

@REM if !USE_MSVC!==1 (
@REM     SET DEFS=/DOS_WINDOWS

@REM     cl /nologo /Zi !SRC! !DEFS! /EHsc /I include /I libs/tracy-0.10/public /FI pch.h /Fobin\ /link /DEBUG /OUT:app.exe
@REM     SET error=!errorlevel!

@REM ) else if !USE_GCC!==1 (
@REM     SET DEFS=-DOS_WINDOWS

@REM     g++ -g !SRC! !DEFS! -Iinclude -include include/pch.h -o app.exe
@REM     SET error=!errorlevel!
@REM )

@REM if !errorlevel!==0 (
@REM     echo f | xcopy bin\app.exe app.exe /y /q > nul
@REM     app
@REM )
