// ********************************************************
//
//   Author/Copyright 	Gero Kuehn / GkWare e.K.
//						Hatzper Strasse 172B
//						D-45149 Essen, Germany
//						Tel: +49 174 520 8026
//						Email: support@gkware.com
//						Web: http://www.gkware.com
//
//
// ********************************************************
#ifndef __KERNEL_H
#define __KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SAFE_FREE
	//! If the free() implementation on platform does not like NULL-pointers, then
	//! SAFE_FREE(x) should be defined as: "if(x) { free(x); x=0; }"
	//! Otherwise "{ free(x); x=0; }" is sufficient.
#define SAFE_FREE(x) if(x) { free(x); x=0; }
#endif

typedef enum {
	TASK_PRIO_LOADCHECK		= 0, // reserved vor CPU load measurement ! 
	TASK_PRIO_IDLE			= 1, 
	TASK_PRIO_LOWEST		= 10, 
	TASK_PRIO_LOWER			= 20, 
	TASK_PRIO_LOW			= 30, 
	TASK_PRIO_BELOW_NORMAL	= 40, 
	TASK_PRIO_NORMAL		= 50, 
	TASK_PRIO_ABOVE_NORMAL	= 60, 
	TASK_PRIO_HIGH			= 70, 
	TASK_PRIO_HIGHER		= 80, 
	TASK_PRIO_HIGHEST		= 99 
} TASK_PRIORITY;



//! defines a default priority
#define TASK_PRIO_DEFAULT ((TASK_PRIORITY)TASK_PRIO_NORMAL)

//! GURU_Init is called by the low level initialization code before any other function from the kernel.
void GURU_Init(void);

typedef enum {
	GURU_PI_INVALID = 0,	//!< invalid marker
	GURU_PI_CHIPTYPE,		//!< the chip
	GURU_PI_BOARDREV,		//!< The board version (if known)
	GURU_PI_COMPILER,		//!< The compiler name and version
	GURU_PI_BUILDTYPE,		//!< "Debug" or "Release"
	GURU_PI_KERNELVER, 		//!< The linux or Windows kernel version
	GURU_PI_NUMCORES		//!< The number of available CPU cores
} GURU_PLATFORMINFO;

const char *GURU_GetPlatformInfo(GURU_PLATFORMINFO eInfoType);


#define GURU_RESULT int

#define MSTOTICKS(x) x
#define TICKSTOMS(x) x


///////////////////////////////////////////////////////
// Tasks
typedef void (*TASKSTARTFUNC)(void *pArgument);

#define TASK_HANDLE pthread_t
#define INVALID_TASK_HANDLE -1

//! GURU_CreateTaskNamed create a new task and starts it.
//! @param pFunc Entry point of the new task
//! @param Prio Priority of the new task
//! @param dwStackSize Minimum stack size that has to be allocated for this task
//! @param pArgument User data pointer that will be passed to the task entry point
//! @param pszName Name of the new Task
//! @return A handle to the new task or INVALID_TASK_HANDLE in case of an error.
//!         The numerical value of INVALID_TASK_HANDLE is platform dependent.
TASK_HANDLE GURU_CreateTaskNamed(TASKSTARTFUNC pFunc,TASK_PRIORITY Prio, DWORD dwStackSize, void *pArgument, const char *pszName);

#define GURU_CreateTask(pFunc,Prio,dwStackSize,pArgument) GURU_CreateTaskNamed(pFunc,Prio,dwStackSize,pArgument, #pFunc )

//! Returns a handle to the currently running task
//! @return Handle to the currently running task
TASK_HANDLE GURU_ThisTask(void);

// Suspends/Resumes a task, returns TRUE on success
//#define GURU_SuspendTask(hTask) SuspendThread(hTask)
//#define GURU_ResumeTask(hTask) ResumeThread(hTask)

// Changes a task priority, returns TRUE on success
//#define GURU_SetPriority(hTask,Prio) SetThreadPriority(hTask,Prio);

//! GURU_Delay delays execution of the current task for a number of clock ticks
//! @param dwTicks Number of ticks
void GURU_Delay(DWORD dwTicks);

// Deletes a task
//#define GURU_DeleteTask(hTask) ExitThread(0)
void GURU_DeleteTask(TASK_HANDLE hTask);




///////////////////////////////////////////////////////
// Semaphores
#include <semaphore.h>

#define SEMAPHORE_HANDLE sem_t*
#define INVALID_SEMAPHORE_HANDLE NULL
typedef DWORD SEMAPHORE_RESULT;
#define GURU_SEMAPHORE_WAIT_INFINITE	0xFFFFFFFF
#define GURU_SEMAPHORE_TIMEOUT			1
#define GURU_SEMAPHORE_OK				2
#define GURU_SEMAPHORE_UNAVAILABLE		3

//! GURU_CreateSemaphore creates a semaphore handle
//! @param dwInitialCount Initial count of the semaphore
//! @return A valid semaphore handle or INVALID_SEMAPHORE_HANDLE if an error occured.
//!         The numerical value of INVALID_SEMAPHORE_HANDLE is platform dependent.
SEMAPHORE_HANDLE GURU_CreateSemaphore(DWORD dwInitialCount);

//! GURU_SemaphoreWait waits for a semaphore with a timeout.
//! This function can NOT be called from an ISR.
//! @param hSema A valid semaphore handle
//! @param dwWaitTicks The number of clock ticks to wait.
//!                    The value 0 should not be used (use GURU_SemaphoreTry() instead).
//!                    GURU_SEMAPHORE_WAIT_INFINITE (platform dependent constant) can be used for infinite waits.
//! @return The numerical values of these constants are platform dependent:
//!         - GURU_SEMAPHORE_OK if the semaphore was obtained successfully
//!         - GURU_SEMAPHORE_TIMEOUT if a timeout occured.
//!         - GURU_SEMAPHORE_UNAVAILABLE if the wait operation failed for any reason
//!         
SEMAPHORE_RESULT GURU_SemaphoreWait(SEMAPHORE_HANDLE hSema, DWORD dwWaitTicks);


//! GURU_SemaphoreTry tries to get a semaphore lock without waiting
//! @param hSema A valid semaphore handle
//! @return see GURU_SemaphoreWait()
SEMAPHORE_RESULT GURU_SemaphoreTry(SEMAPHORE_HANDLE hSema);


//! GURU_SemaphoreRelease increases the count of the semaphore by one.
//! If more than one task is waiting for the semaphore, the order in which they are released is
//! undefined.
//! @param hSema A valid semaphore handle
void GURU_SemaphoreRelease(SEMAPHORE_HANDLE hSema);

//! GURU_SemaphoreDelete releases a semaphore that was allocated using GURU_CreateSemaphore()
//! The behaviour of tasks that are currently waiting for the semaphore is undefined.
//! @param hSema A valid semaphore handle
void GURU_SemaphoreDelete(SEMAPHORE_HANDLE hSema);


///////////////////////////////////////////////////////
// Mutexes
#define MUTEX_HANDLE pthread_mutex_t

//! GURU_CreateMutex Initializes a Mutex/Critical-Section object.
//! @return TRUE if the object was initialized correctly, FALSE if an error occurred.
BOOL GURU_CreateMutex(MUTEX_HANDLE *pMutex);

//! GURU_Mutex_Lock acquires a mutex/enters a critical section.
//! Recursive locking from the same thread context must be supported (that is the difference to a semaphore).
//! This function locks until the mutex is available.
void GURU_Mutex_Lock(MUTEX_HANDLE *pMutex);

//! GURU_Mutex_Lock releases a mutex/leaves a critical section.
void GURU_Mutex_Unlock(MUTEX_HANDLE *pMutex);

//! GURU_Mutex_Delete releases all resources attached to a mutex/critical section object.
void GURU_Mutex_Delete(MUTEX_HANDLE *pMutex);


///////////////////////////////////////////////////////
// Queues
#ifdef __i386__
typedef DWORD HAL_QUEUE_MSGTYPE;
#elif __amd64__
typedef QWORD HAL_QUEUE_MSGTYPE;
#elif __mips__
typedef DWORD HAL_QUEUE_MSGTYPE;
#else
#error
#endif


typedef struct {
	SEMAPHORE_HANDLE hLock;
	SEMAPHORE_HANDLE hMsgSema;
	int nMessages;
	HAL_QUEUE_MSGTYPE *pData;
	DWORD dwMaxCount;
} LINUXQUEUE;

#define QUEUE_HANDLE LINUXQUEUE*
#define INVALID_QUEUE_HANDLE		NULL
#define GURU_QUEUE_WAIT_INFINITE	0xFFFFFFFF
#define GURU_QUEUE_TIMEOUT			1
#define GURU_QUEUE_EMPTY			2
#define GURU_QUEUE_FULL				3
#define GURU_QUEUE_OK				4

//! GURU_CreateQueue allocates a new queue handle. Queues are a mechanism to send messages
//! that consist of 4 DWORDs betweens tasks.
//! @param dwMaxCount The maximum number of messages that the queue can hold.
//! @return A new queue handle or INVALID_QUEUE_HANDLE in case of an error.
//!         The numerical value of INVALID_QUEUE_HANDLE is platform dependent.
QUEUE_HANDLE GURU_CreateQueue(DWORD dwMaxCount);

//! GURU_QueueRecv receives a message from a queue.
//! @param hQueue A valid queue handle
//! @param msg The 4 DWORDs are copied into the msg buffer if the call succeeds.
//! @param dwWaitTicks The number of clock ticks to wait for a message.
//!                    GURU_QUEUE_WAIT_INFINITE can be used for infinite waits.
//!                    The numerical value of GURU_QUEUE_WAIT_INFINITE is platform dependent.
//! @return The numerical values of these constants are platform dependent:
//!         - GURU_QUEUE_OK A message arrived and has been placed into the msg buffer
//!         - GURU_QUEUE_TIMEOUT The receive operation timed out
int GURU_QueueRecv(QUEUE_HANDLE hQueue, HAL_QUEUE_MSGTYPE msg[4], DWORD dwWaitTicks);


//! GURU_QueueRecv tries to receive a message from a queue.
//! @param hQueue A valid queue handle
//! @param msg The 4 DWORDs are copied into the msg buffer if the call succeeds.
//! @return The numerical values of these constants are platform dependent:
//!         - GURU_QUEUE_OK A message arrived and has been placed into the msg buffer
//!         - GURU_QUEUE_EMPTY The queue is empty
int GURU_QueueTryRecv(QUEUE_HANDLE hQueue,HAL_QUEUE_MSGTYPE msg[4]);

//! GURU_QueueRecv sends a message to a queue.
//! @param hQueue A valid queue handle
//! @param msg The message to send. The 4 DWORDs are copied into the QUEUE if the call succeeds.
//! @return The numerical values of these constants are platform dependent:
//!			- GURU_QUEUE_OK The message was sent successfully
//!         - GURU_QUEUE_FULL The queue is full. The message was not sent.
int GURU_QueueSend(QUEUE_HANDLE hQueue, const HAL_QUEUE_MSGTYPE msg[4]);

//! GURU_QueueDelete deletes a queue that has been allocated using GURU_CreateQueue().
//! No tasks should be waiting in GURU_QueueRecv for the handle when this function is called.
//! Otherwise the behaviour is undefined.
void GURU_QueueDelete(QUEUE_HANDLE hQueue);

#define GURU_STANDBY_INFINITE 0xFFFFFFFE
void GURU_EnterHardwareStandby(DWORD dwWakeupSecs);

//! WAKEUP_REASON defines the various reasons for a system to start/restart
typedef enum {
	WAKEUP_REASON_UNKNOWN = 0,		//!< feature unsupported by HAL
	WAKEUP_REASON_COLDBOOT = 1,		//!< system power cycle
	WAKEUP_REASON_IRCOMMAND = 2,	//!< infrared command
	WAKEUP_REASON_FRONTPANEL = 3,	//!< frontpanel button
	WAKEUP_REASON_TIMER = 4,		//!< timer expiration
	WAKEUP_REASON_WAKE_ON_LAN = 5   //!< wake on lan event
} WAKEUP_REASON;

WAKEUP_REASON GURU_GetWakeupReason(void);

//! GURU_GetTickCount returns the current value of the system clock tick counter.
//! @return the current clock tick counter value
DWORD GURU_GetTickCount();

void GURU_Reset(void);

//! GURU_GetKernelCMDLine returns the commandline of the linux kernel (or application) on platforms where such a commandline exists
//! @return The commandline as a zero-terminated string. The returned buffer is static, probably initialized on the first call and does not need to be freed by anyone.
//! The return value is NULL or an empty string on platforms not supporting this feature.
const char *GURU_GetKernelCMDLine(void);

//! GURU_SetOSTime sets the supplied time as system time for the operating system.
//! The OS time influences timestamps on files written to attached media devices.
//! @param t The time to set as new system time.
void GURU_SetOSTime(time_t t);


//! GURU_GenerateRandom tries to obtain from a hardware random source or an operating-system supplied mechanism.
//! Data produced by this function should be safe to use for cryptographic purposes.
//! The function must fail (return FALSE) if insufficient entropy is available or the RNG hardware fails.
//! Incorrect implementation of this function can lead to severe cryptography problems.
//! @param pOut Pointer to the buffer that should be filled with random data.
//! @param size The number of bytes to write.
//! @return TRUE if the requested number of random bytes was written, FALSE in any other case.
BOOL GURU_GenerateRandom(BYTE *pOut, size_t size);


#ifdef __cplusplus
}
#endif


#endif // __KERNEL_H
