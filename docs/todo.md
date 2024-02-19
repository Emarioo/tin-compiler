# Overview
- Command line options (file to compile, logging on/off, interpreter yes/no, interactive interpreter yes/no)
- Types, pointers, structs
- Verify return types, arguments and such
- Include
- Reference, dereference operator (since we have pointers)
- sizeof TYPE
- Multithreading
- Native functions (print, read, write file)
- Procedural source code generation (test compiler performance with large data)

# Extra
- Index operator `ptr[4]` instead of `*(ptr + 4)`

# Details about overview
## Command line options
Specify file like this: `comp main.txt`
Compile file and run interpreter in interactive mode: `comp main.txt -run -interactive`
Turn on logging: `comp main.txt -log`. Maybe you can choose which parts of the compiler to log.

