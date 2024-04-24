/*
    This file is used to change the way the compiler compiles such
    as enabling/disabling multithreading.

    Comment/uncomment macros to toggle the behaviour

    Do not comment out the #ifndef DISABLE_X
    They allow you to specify macros to enable or disable
    multithreading from the command line "g++ /DDISABLE_X"
*/

#ifndef DISABLE_MULTITHREADING
// #define ENABLE_MULTITHREADING
#endif