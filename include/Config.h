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

#ifndef DISABLE_OPTIMIZATIONS
#define ENABLE_OPTIMIZATIONS
#endif

// TURN THIS OFF DURING PERFORMANCE TESTING
// #define ENABLE_ALLOCATION_TRACKER

/*###########################
    TOGGLE OPTIMIZATIONS
##########################*/

#ifdef ENABLE_OPTIMIZATIONS

#define DOUBLE_AST_ARRAYS

// Mutexes when adding and looking for identifiers are removed.
// We know that no two threads (unless global scope) will access
// local scopes from the same import at the same time.
#define IGNORE_UNNECESSARY_MUTEXES

#define DISABLE_ASSERTS

#define AVOID_STDLIB
#endif