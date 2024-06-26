#pragma once

#include "tracy/Tracy.hpp"
#include "Config.h"
#include <typeinfo>

// Added pch.h here because VSCode broke and ignores
// forced include files from Win32 configuration.
#include "pch.h"

#ifndef DISABLE_ASSERTS
#define Assert(E) ((bool)(E) || (fprintf(stderr,"[ASSERT %s:%d]: %s\n",__FILE__, __LINE__, #E), (bool)(*(int*)nullptr = 9)))
#else
#define Assert(E)
#endif
struct DeferFunc {
    ~DeferFunc() { f(); }
    std::function<void()> f;
};
#define defer DeferFunc _defer; _defer.f = [&]()

enum Color : u8 {
    BLACK = 0x00,
    NAVY = 0x01,
    GREEN = 0x02,
    CYAN = 0x03,
    BLOOD = 0x04,
    PURPLE = 0x05,
    GOLD = 0x06,
    SILVER = 0x07,
    GRAY = 0x08,
    BLUE = 0x09,
    LIME = 0x0A,
    AQUA = 0x0B,
    RED = 0x0C,
    MAGENTA = 0x0D,
    YELLOW = 0x0E,
    WHITE = 0x0F,
    _BLACK = 0x00,
    _BLUE = 0x10,
    _GREEN = 0x20,
    _TEAL = 0x30,
    _BLOOD = 0x40,
    _PURPLE = 0x50,
    _GOLD = 0x60,
    _SILVER = 0x70,
    _GRAY = 0x80,
    _NAVY = 0x90,
    _LIME = 0xA0,
    _AQUA = 0xB0,
    _RED = 0xC0,
    _MAGENTA = 0xD0,
    _YELLOW = 0xE0,
    _WHITE = 0xF0,
    NO_COLOR = 0x00,
};

void log_color(Color color);

/*###############################################
    THREAD, MUTEX, AND SEMAPHORE UTILITIES
###############################################*/

typedef u64 ThreadId;
typedef u32 TLSIndex;
class Thread {
public:
    Thread() = default;
    // ~Thread(){ cleanup(); } // no destructor, in case you resize arrays and cause move semantics BUT don't make
    // The thread itself should not call this function
    void cleanup();
    
    void init(u32(*func)(void*), void* arg);
    // joins and destroys the thread
    void join();
    
    // True: Thread is doing stuff or finished and waiting to be joined.
    // False: Thread is not linked to an os thread. Call init to start the thread.
    bool isActive();
    bool joinable();
    ThreadId getId();

    static ThreadId GetThisThreadId();

    // Thread local storage (or thread specific data)
    // 0 indicates failure
    static TLSIndex CreateTLSIndex();
    static bool DestroyTLSIndex(TLSIndex index);

    static void* GetTLSValue(TLSIndex index);
    static bool SetTLSValue(TLSIndex index, void* ptr);

private:
    static const int THREAD_SIZE = 8;
    union {
        struct { // Windows
            u64 m_internalHandle;
        };
        struct { // Unix
            u64 m_data;
        };
        u64 _zeros = 0;
    };
    ThreadId m_threadId=0;
    
    friend class FileMonitor;
};
class Semaphore {
public:
    Semaphore() = default;
    Semaphore(u32 initial, u32 maxLocks);
    ~Semaphore() { cleanup(); }
    void cleanup();

    void init(u32 initial, u32 maxLocks);

    void wait();
    // count is how much to increase the semaphore's count by.
    void signal(int count=1);

private:
    u32 m_initial = 1;
    u32 m_max = 1;
    static const int SEM_SIZE = 32;
    union {
        struct { // Windows
            u64 m_internalHandle;
        };
        struct { // Unix
            u8 m_data[SEM_SIZE]; // sem_t
            bool m_initialized;
        };
        u8 _zeros[SEM_SIZE+1]{0}; // intelisense doesn't like multiple initialized fields even though they are in anonmymous struct, sigh. So we use one field here.
    };
};
class Mutex {
public:
    Mutex() = default;
    ~Mutex(){cleanup();}
    void cleanup();

    void lock();
    void unlock();

    ThreadId getOwner();
private:
    ThreadId m_ownerThread = 0;
    u64 m_internalHandle = 0;
};

typedef u64 TimePoint;
TimePoint StartMeasure();
// returns time in seconds
double StopMeasure(TimePoint startPoint);
// example: DiffMeasure(StartMeasure() - StartMeasure())
double DiffMeasure(TimePoint endSubStart);
// Returns value in Hz
u64 GetClockSpeed();
void SleepMS(int ms);

int RandomInt(int min, int max);
float RandomFloat();
void SetRandomSeed(int seed);

bool ReadEntireFile(const std::string& path, char*& text, int& size);

#ifdef ENABLE_MULTITHREADING
    #define MUTEX_DECL(var)   Mutex var{};
    #define MUTEX_LOCK(var)   var.lock();
    #define MUTEX_UNLOCK(var) var.unlock();

    #define SEM_DECL(var)               Semaphore var{};
    #define SEM_INIT(var,initial,max)   var.init(initial,max);
    #define SEM_WAIT(var)               var.wait();
    #define SEM_SIGNAL(var)             var.signal();
#else
    #define MUTEX_DECL(var)  
    #define MUTEX_LOCK(var)  
    #define MUTEX_UNLOCK(var)

    #define SEM_DECL(var)  
    #define SEM_INIT(var,initial,max)
    #define SEM_WAIT(var)  
    #define SEM_SIGNAL(var)
#endif
// returns the result
i32 atomic_add(volatile i32* ptr, i32 value);

struct Location {
    const char* file;
    int line;
};

void* Alloc(int size, Location loc, const std::type_info& type, int type_size);
void* Realloc(int new_size, void* ptr, int old_size, Location loc, const std::type_info& type, int type_size);
void Free(void* ptr, int old_size, Location loc, const std::type_info& type);

int GetAllocatedMemory();
void PrintMemoryUsage(int expected_memory_usage);

#define HERE Location{__FILE__,__LINE__}

#define NEW_ARRAY(TYPE,COUNT,LOC) ((TYPE*)Alloc(sizeof(TYPE) * COUNT,LOC, typeid(TYPE),sizeof(TYPE)))
#define RENEW_ARRAY(OLD_PTR,OLD_COUNT,TYPE,COUNT,LOC) ((TYPE*)Realloc(sizeof(TYPE) * COUNT,OLD_PTR,OLD_COUNT,LOC, typeid(TYPE),sizeof(TYPE)))
#define DELNEW_ARRAY(PTR,TYPE,COUNT,LOC) (Free(PTR, sizeof(TYPE) * COUNT,LOC,typeid(TYPE)))
#define SCALAR_NEW(PTR,TYPE,COUNT) for(int si=0;si<COUNT;si++) new(PTR + si)TYPE();
#define SCALAR_DELNEW(PTR,TYPE,COUNT) for(int si=0;si<COUNT;si++) (PTR + si)->~TYPE();

#define NEW(TYPE,LOC,...) (new((TYPE*)Alloc(sizeof(TYPE),LOC,typeid(TYPE), sizeof(TYPE)))TYPE(__VA_ARGS__))
#define DELNEW(PTR,TYPE,LOC) (PTR->~TYPE(),Free(PTR, sizeof(TYPE),LOC,typeid(TYPE)))


void SetHighProcessPriority();
void SetHighThreadPriority();