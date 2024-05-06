/*
    This file is used to change the way the compiler compiles such
    as enabling/disabling multithreading.

    Comment/uncomment macros to toggle the behaviour

    Do not comment out the #ifndef DISABLE_X
    They allow you to specify macros to enable or disable
    multithreading from the command line "g++ /DDISABLE_X"

Speeds
 4.391

 
 642.085

*/

#ifndef DISABLE_MULTITHREADING
#define ENABLE_MULTITHREADING
#endif


#ifndef DISABLE_OPTIMIZATIONS
#define ENABLE_OPTIMIZATIONS
#endif


/*###########################
    TOGGLE OPTIMIZATIONS
##########################*/

#ifdef ENABLE_OPTIMIZATIONS

// Fixed array of scopes/types (65000), no need to lock the array when adding or accessing
// #define PREALLOCATED_AST_ARRAYS
// exlusive to PREALLOCATED_AST_ARRAYS
#define DOUBLE_AST_ARRAYS

// Mutexes when adding and looking for identifiers are removed.
// We know that no two threads (unless global scope) will access
// local scopes from the same import at the same time.
#define IGNORE_UNNECESSARY_MUTEXES

#define DISABLE_ASSERTS

#define AVOID_STDLIB
#endif