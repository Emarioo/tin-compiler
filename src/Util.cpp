#include "Util.h"

#ifdef OS_WINDOWS
// tracy doesn't want lean and mean
// #define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include <intrin.h>

#define USE_RDTSC

#ifdef PL_PRINT_ERRORS
#define PL_PRINTF printf
#else
#define PL_PRINTF
#endif

#define TO_INTERNAL(X) ((u64)X+1)
#define TO_HANDLE(X) (HANDLE)((u64)X-1)

// Won't work for large files, slow

struct Allocation {
    int size; // size/type_size = element count
    Location loc;
    int type_size = 0;
    const char* type_name = nullptr;
    int alloc_id;
};
std::unordered_map<void*, Allocation> allocations; // TODO: Mutex on this?
MUTEX_DECL(lock_allocations)

static volatile int allocated_memory = 0;
static volatile int allocation_id = 0;
static int alloc_ids[]{ // TODO: Read these from a file?
    0,
    // 916
};
void* Alloc(int size, Location loc, const std::type_info& type, int type_size) {
    auto ptr = malloc(size);
    atomic_add(&allocated_memory, size);
#ifdef ENABLE_ALLOCATION_TRACKER
    MUTEX_LOCK(lock_allocations)
    auto pair = allocations.find(ptr);
    if(pair != allocations.end()) {
        Assert(false); // if malloc returns a pointer that exists in allocatins then
        // we freed the pointer without removing it from allocations
    }
    auto& it = (allocations[ptr] = {});
    it.loc = loc;
    it.size = size;
    it.type_name = type.name();
    it.type_size = type_size;
    it.alloc_id = atomic_add(&allocation_id, 1);
    for(int i=0;i<sizeof(alloc_ids)/sizeof(*alloc_ids);i++) {
        if(it.alloc_id == alloc_ids[i])
            __debugbreak();
    }
    MUTEX_UNLOCK(lock_allocations)
#endif
    return ptr;
}
void* Realloc(int new_size, void* ptr, int old_size, Location loc, const std::type_info& type, int type_size) {
    if(!ptr) {
        return Alloc(new_size, loc, type, type_size);
    }

    auto np = realloc(ptr, new_size);
    atomic_add(&allocated_memory, new_size - old_size);
#ifdef ENABLE_ALLOCATION_TRACKER
    MUTEX_LOCK(lock_allocations)
    auto pair = allocations.find(ptr);
    if(pair == allocations.end()) {
        Assert(false);
    }
    if(np == ptr) {
        auto& it = pair->second;
        it.loc = loc;
        if(it.size != old_size) {
            printf("%s:%d\n",loc.file,loc.line);
        }
        Assert(it.size ==  old_size);
        it.size = new_size;
        Assert(0 == strcmp(it.type_name, type.name()));
    } else {
        Assert(pair->second.size == old_size);
        Assert(0 == strcmp(pair->second.type_name, type.name()));

        auto& it (allocations[np] = { });
        it.loc = loc;
        it.size = new_size;
        it.type_name = type.name();
        it.type_size = pair->second.type_size;
        it.alloc_id = atomic_add(&allocation_id, 1);
        
        for(int i=0;i<sizeof(alloc_ids)/sizeof(*alloc_ids);i++) {
            if(it.alloc_id == alloc_ids[i])
                __debugbreak();
        }

        allocations.erase(ptr);
    }
    MUTEX_UNLOCK(lock_allocations)
#endif
    return np;
}
void Free(void* ptr, int old_size, Location loc, const std::type_info& type) {
    #ifdef ENABLE_ALLOCATION_TRACKER
    MUTEX_LOCK(lock_allocations)
    auto pair = allocations.find(ptr);
    if(pair == allocations.end()) {
        Assert(false);
    }
    Assert(pair->second.size == old_size);
    Assert(0 == strcmp(pair->second.type_name, type.name()));
    allocations.erase(ptr);
    MUTEX_UNLOCK(lock_allocations)
    #endif
    free(ptr);
    atomic_add(&allocated_memory, -old_size);
}
int GetAllocatedMemory() {
    return allocated_memory;
}
void PrintMemoryUsage(int expected_memory_usage) {
    int memory = allocated_memory - expected_memory_usage;
    if(allocations.size() == 0) {
        if(memory != 0) {
            log_color(YELLOW);
            printf("Memory leaks: no specific tracked pointers, but %d bytes are floating, %d bytes are expected (bug with tracking?)\n", allocated_memory, expected_memory_usage);
            log_color(NO_COLOR);
        } else {
            log_color(YELLOW);
            printf("Memory leaks: none");
            log_color(NO_COLOR);
        }
        return;
    }
    // TODO: Format or sort the leaks
    log_color(RED);
    printf("Memory leaks (%d b, expected %d):\n", allocated_memory, expected_memory_usage);
    log_color(GRAY);
    printf("(below are current allocations, all may not be memory leaks)\n");
    log_color(NO_COLOR);
    for(auto& pair : allocations) {
        auto& it = pair.second;
        log_color(AQUA);
        printf(" %d ",it.alloc_id);
        log_color(GREEN);
        printf(" %s (%d b, %d elems)",it.type_name, it.type_size, it.size / it.type_size);
        printf(" ");
        log_color(GRAY);
        std::string path = it.loc.file;
        int index = path.find("src");
        path = path.substr(index);
        printf("%s:%d",path.c_str(),it.loc.line);
        printf("\n");
        log_color(NO_COLOR);
    }
}
bool ReadEntireFile(const std::string& path, char*& text, int& size){
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h == INVALID_HANDLE_VALUE) return false;

    u64 filesize = 0;
    DWORD success = GetFileSizeEx(h, (LARGE_INTEGER*)&filesize);
    if(success == 0) return false;

    text = (char*)NEW_ARRAY(char, filesize, HERE);
    Assert(text);

    success = ReadFile(h, text, filesize, (DWORD*)&size, NULL);
    CloseHandle(h);
    return success;
}
TimePoint StartMeasure(){
    #ifdef USE_RDTSC
    return (u64)__rdtsc();
    #else
    u64 tp;
    BOOL success = QueryPerformanceCounter((LARGE_INTEGER*)&tp);
    // if(!success){
    // 	PL_PRINTF("time failed\n");	
    // }
    // TODO: handle err
    return tp;
    #endif
}
static bool once = false;
static u64 frequency;
double StopMeasure(TimePoint startPoint){
    if(!once){
        once=true;
        #ifdef USE_RDTSC
        // TODO: Does this work on all Windows versions?
        //  Could it fail? In any case, something needs to be handled.
        DWORD mhz = 0;
        DWORD size = sizeof(DWORD);
        LONG err = RegGetValueA(
            HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "~MHz",
            RRF_RT_DWORD,
            NULL,
            &mhz,
            &size
        );
        frequency = (u64)mhz * (u64)1000000;
        #else
        BOOL success = QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        // if(!success){
        // 	PL_PRINTF("time failed\n");	
        // }
        // TODO: handle err
        #endif
    }
    TimePoint endPoint;
    #ifdef USE_RDTSC
    endPoint = (u64)__rdtsc();
    #else
    BOOL success = QueryPerformanceCounter((LARGE_INTEGER*)&endPoint);
    // if(!success){
    // 	PL_PRINTF("time failed\n");	
    // }
    #endif
    return (double)(endPoint-startPoint)/(double)frequency;
}
double DiffMeasure(TimePoint endSubStart) {
    if(!once){
        once=true;
        #ifdef USE_RDTSC
        // TODO: Does this work on all Windows versions?
        //  Could it fail? In any case, something needs to be handled.
        DWORD mhz = 0;
        DWORD size = sizeof(DWORD);
        LONG err = RegGetValueA(
            HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "~MHz",
            RRF_RT_DWORD,
            NULL,
            &mhz,
            &size
        );
        frequency = (u64)mhz * (u64)1000000;
        #else
        BOOL success = QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        // if(!success){
        // 	PL_PRINTF("time failed\n");	
        // }
        // TODO: handle err
        #endif
    }
    return (double)(endSubStart)/(double)frequency;
}
u64 GetClockSpeed(){
    static u64 clockSpeed = 0; // frequency
    if(clockSpeed==0){
        // TODO: Does this work on all Windows versions?
        //  Could it fail? In any case, something needs to be handled.
        DWORD mhz = 0;
        DWORD size = sizeof(DWORD);
        LONG err = RegGetValueA(
            HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "~MHz",
            RRF_RT_DWORD,
            NULL,
            &mhz,
            &size
        );
        clockSpeed = (u64)mhz * (u64)1000000;
    }
    return clockSpeed;
}
void SleepMS(int ms) {
    Sleep(ms);
}

static std::mt19937 s_random_gen = std::mt19937(time(0));
void SetRandomSeed(int seed) {
    s_random_gen = std::mt19937(seed);
}
// possible values include the value of max. Distribution = [min, max + 1]
int RandomInt(int min, int max) {
    if(min == max) return min;
    Assert(min <= max);
    std::uniform_int_distribution<int> dist(min, max);
    return dist(s_random_gen);
}
float RandomFloat() {
    std::uniform_real_distribution<float> dist(0.f, 1.0f);
    return dist(s_random_gen);
}

void log_color(Color color){
    if(color == Color::NO_COLOR)
        color = Color::SILVER;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, (u16)color);
}

i32 atomic_add(volatile i32* ptr, i32 value) {
    i32 res = InterlockedAdd((volatile long*)ptr, value);
    return res;
}
Semaphore::Semaphore(u32 initial, u32 max) {
    Assert(!m_internalHandle);
    m_initial = initial;
    m_max = max;
}
void Semaphore::init(u32 initial, u32 max) {
    Assert(!m_internalHandle);
    m_initial = initial;
    m_max = max;
    HANDLE handle = CreateSemaphore(NULL, m_initial, m_max, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] CreateSemaphore\n",err);
    } else {
        m_internalHandle = TO_INTERNAL(handle);
    }
}
void Semaphore::cleanup() {
    if (m_internalHandle != 0){
        BOOL yes = CloseHandle(TO_HANDLE(m_internalHandle));
        if(!yes){
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CloseHandle\n",err);
        }
        m_internalHandle=0;
    }
}
void Semaphore::wait() {
    // MEASURE
    if (m_internalHandle == 0) {
        HANDLE handle = CreateSemaphore(NULL, m_initial, m_max, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CreateSemaphore\n",err);
        } else {
            m_internalHandle = TO_INTERNAL(handle);
        }
    }
    if (m_internalHandle != 0) {
        DWORD res = WaitForSingleObject(TO_HANDLE(m_internalHandle), INFINITE);
        if (res == WAIT_FAILED) {
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] WaitForSingleObject\n",err);
        }
    }
}
void Semaphore::signal(int count) {
    // MEASURE
    if (m_internalHandle != 0) {
        BOOL yes = ReleaseSemaphore(TO_HANDLE(m_internalHandle), count, NULL);
        if (!yes) {
            DWORD err = GetLastError();
            if(err == ERROR_TOO_MANY_POSTS)
                PL_PRINTF("[WinError %lu] ReleaseSemaphore, to many posts\n",err);
            else
                PL_PRINTF("[WinError %lu] ReleaseSemaphore\n",err);
        }
    }
}
void Mutex::cleanup() {
    if (m_internalHandle != 0){
        BOOL yes = CloseHandle(TO_HANDLE(m_internalHandle));
        if(!yes){
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CloseHandle\n",err);
        }
        m_internalHandle=0;
    }
}
void Mutex::lock() {
    // MEASURE
    ZoneScopedC(tracy::Color::Bisque4);
    if (m_internalHandle == 0) {
        HANDLE handle = CreateMutex(NULL, false, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CreateMutex\n",err);
        }else
            m_internalHandle = TO_INTERNAL(handle);
    }
    if (m_internalHandle != 0) {
        DWORD res = WaitForSingleObject(TO_HANDLE(m_internalHandle), INFINITE);
        u32 newId = Thread::GetThisThreadId();
        auto owner = m_ownerThread;
        // printf("Lock %d %d\n",newId, (int)m_internalHandle);
        if (owner != 0) {
            PL_PRINTF("Mutex : Locking twice, old owner: %llu, new owner: %u\n",owner,newId);
        }
        m_ownerThread = newId;
        if (res == WAIT_FAILED) {
            // TODO: What happened do the old thread who locked the mutex. Was it okay to ownerThread = newId
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] WaitForSingleObject\n",err);
        }
    }
}
void Mutex::unlock() {
    // MEASURE
    if (m_internalHandle != 0) {
        // printf("Unlock %d %d\n",m_ownerThread, (int)m_internalHandle);
        m_ownerThread = 0;
        BOOL yes = ReleaseMutex(TO_HANDLE(m_internalHandle));
        if (!yes) {
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] ReleaseMutex\n",err);
        }
    }
}
ThreadId Mutex::getOwner() {
    return m_ownerThread;
}
void Thread::cleanup() {
    if (m_internalHandle) {
        if (m_threadId == GetThisThreadId()) {
            PL_PRINTF("Thread : Thread cannot clean itself\n");
            return;
        }
        BOOL yes = CloseHandle(TO_HANDLE(m_internalHandle));
        if(!yes){
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CloseHandle\n",err);
        }
        m_internalHandle=0;
    }
}
struct DataForThread {
    u32(*func)(void*) = nullptr;
    void* userData = nullptr;
    static DataForThread* Create(){
        // Heap allocated at the moment but you could create a bucket array
        // instead. Or store 40 of these as global data and then use heap
        // allocation if it fills up.
        auto ptr = NEW(DataForThread, HERE);
        return ptr;
    }
    static void Destroy(DataForThread* ptr){
        DELNEW(ptr, DataForThread, HERE);
    }
};
DWORD WINAPI SomeThreadProc(void* ptr){
    DataForThread* data = (DataForThread*)ptr;
    u32 ret = data->func(data->userData);
    DataForThread::Destroy(data);
    return ret;
}
u32 Thread::CreateTLSIndex() {
    DWORD ind = TlsAlloc();
    if(ind == TLS_OUT_OF_INDEXES) {
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] TlsAlloc\n",err);
    }
    return ind+1;
}
bool Thread::DestroyTLSIndex(u32 index) {
    bool yes = TlsFree((DWORD)(index-1));
    if(!yes) {
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] TlsFree\n",err);
    }
    return yes;
}
void* Thread::GetTLSValue(u32 index){
    void* ptr = TlsGetValue((DWORD)(index-1));
    DWORD err = GetLastError();
    if(err!=ERROR_SUCCESS){
        PL_PRINTF("[WinError %lu] TlsGetValue, invalid index\n",err);
    }
    return ptr;
}
bool Thread::SetTLSValue(u32 index, void* ptr){
    bool yes = TlsSetValue((DWORD)(index-1), ptr);
    if(!yes) {
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] TlsSetValue, invalid index\n",err);
    }
    return yes;
}
void Thread::init(u32(*func)(void*), void* arg) {
    if (!m_internalHandle) {
        // const uint32 stackSize = 1024*1024;
        // Casting func to LPTHREAD_START_ROUTINE like this is questionable.
        // func doesn't use a specific call convention while LPTHREAD uses   __stdcall.
        // HANDLE handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0,(DWORD*)&m_threadId);

        // SomeThreadProc is correct, we create a little struct to keep the func ptr and arg in.
        auto ptr = DataForThread::Create();
        ptr->func = func;
        ptr->userData = arg;
        
        HANDLE handle = CreateThread(NULL, 0, SomeThreadProc, ptr, 0,(DWORD*)&m_threadId);
        if (handle==INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            PL_PRINTF("[WinError %lu] CreateThread\n",err);
        }else
            m_internalHandle = TO_INTERNAL(handle);
    }
}
void Thread::join() {
    if (!m_internalHandle)
        return;
    DWORD res = WaitForSingleObject(TO_HANDLE(m_internalHandle), INFINITE);
    if (res==WAIT_FAILED) {
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] WaitForSingleObject\n",err);
    }
    BOOL yes = CloseHandle(TO_HANDLE(m_internalHandle));
    if(!yes){
        DWORD err = GetLastError();
        PL_PRINTF("[WinError %lu] CloseHandle\n",err);
    }
    m_threadId = 0;
    m_internalHandle = 0;
}
bool Thread::isActive() {
    return m_internalHandle!=0;
}
bool Thread::joinable() {
    return m_threadId!=0 && m_threadId != GetCurrentThreadId();
}
ThreadId Thread::getId() {
    return m_threadId;
}
ThreadId Thread::GetThisThreadId() {
    return GetCurrentThreadId();
}

void SetHighProcessPriority() {
    auto h = GetCurrentProcess();
    // auto err = SetPriorityClass(h, HIGH_PRIORITY_CLASS);
    auto err = SetPriorityClass(h, REALTIME_PRIORITY_CLASS); // dangerous?
    if (err == 0) {
        printf("fail prio\n");
    }
}
void SetHighThreadPriority() {
    auto h = GetCurrentThread();
    // auto err = SetThreadPriority(h, THREAD_PRIORITY_HIGHEST);
    auto err = SetThreadPriority(h, THREAD_PRIORITY_TIME_CRITICAL); // dangerous?
    if (err == 0) {
        printf("fail prio\n");
    }
}

#endif

#ifdef OS_UNIX



#endif