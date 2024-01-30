@echo off
@setlocal enabledelayedexpansion

SET SRC= main.cpp Lexer.cpp Parser.cpp AST.cpp Compiler.cpp
SET SRC=!SRC: = src/!

g++ -g !SRC! -Iinclude -include include/pch.h -o app.exe

app


