#include "Util.h"

#ifdef OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include <intrin.h>

#ifdef PL_PRINT_ERRORS
#define PL_PRINTF printf
#else
#define PL_PRINTF
#endif

#define TO_INTERNAL(X) ((u64)X+1)
#define TO_HANDLE(X) (HANDLE)((u64)X-1)

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
        auto ptr = (DataForThread*)malloc(sizeof(DataForThread));
        // auto ptr = (DataForThread*)Allocate(sizeof(DataForThread));
        new(ptr)DataForThread();
        return ptr;
    }
    static void Destroy(DataForThread* ptr){
        ptr->~DataForThread();
        free(ptr);
        // Free(ptr,sizeof(DataForThread));
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
#endif

#ifdef OS_UNIX



#endif