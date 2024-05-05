#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include <random>
#include <string>
#include <math.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;


// Can't have this here
// Windows defines TokenKind which we call our enum
// We could undefine TokenKind but let's not go down that route
// #ifdef OS_WINDOWS
//     #define WIN32_LEAN_AND_MEAN
//     #include "Windows.h"
//     #include <intrin.h>
// #endif