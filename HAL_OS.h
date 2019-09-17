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
#ifndef __HAL_OS_H
#define __HAL_OS_H

#ifdef __cplusplus
extern "C" {
#endif

//! @file HAL_OS.h
//! @brief Basic HAL & Operating System mapping.
//! For performance reasons, some of the functions in this API 
//! may be replaced for by macros that wrap to native OS functions.
//! The actual values of enums and \#defines also depend on the platform.
//! The PC HAL is using exactly the definitions in this file.
//! The platform.h should include either also this file
//! or it must include an alternative file with compatible
//! alternative functions, macros and enums.

///////////////////////////////////////////////////////
// General definitions , valid for all platforms

#ifndef SAFE_FREE
//! If the free() implementation on platform does not like NULL-pointers, then
//! SAFE_FREE(x) should be defined as: "if(x) { free(x); x=0; }"
//! Otherwise "{ free(x); x=0; }" is sufficient.
#define SAFE_FREE(x) if(x) { free(x); x=0; }
#endif

//! TASKSTARTFUNC defines what a task entry point looks like
//! This type is used as parameter to GURU_CreateTask()
//! When this function returns, the task should not automatically restart.
//! @param pArgument The argument pointer passed to GURU_CreateTask()
typedef void (*TASKSTARTFUNC)(void *pArgument);

//! TASK_PRIORITY defines the priority of a task.
//! Code must always use the enum names. The numerical values are platform dependent.
typedef enum {
  TASK_PRIO_IDLE = 10,			//!< task only runs when no other task is busy
  TASK_PRIO_LOWEST = 20,		//!< lowest non-idle priority level
  TASK_PRIO_LOW = 30,			//!< lower than normal
  TASK_PRIO_BELOW_NORMAL = 40,  //!<
  TASK_PRIO_NORMAL = 50,		//!< default priority level
  TASK_PRIO_ABOVE_NORMAL = 60,  //!<
  TASK_PRIO_HIGH = 100,         //!< higher than normal
  TASK_PRIO_HIGHEST = 230		//!< highest priority level
} TASK_PRIORITY;


//! defines a default priority
#define TASK_PRIO_DEFAULT ((TASK_PRIORITY)TASK_PRIO_NORMAL)

//! GURU_Init is called by the low level initialization code before any other function of the kernel API.
void GURU_Init(void);

typedef enum {
	GURU_PI_INVALID = 0,	//!< invalid marker
	GURU_PI_CHIPTYPE,		//!< the chip
	GURU_PI_BOARDREV,		//!< The board version (if known)
	GURU_PI_COMPILER,		//!< The compiler name and version
	GURU_PI_BUILDTYPE,		//!< "Debug" or "Release"
	GURU_PI_KERNELVER,		//!< The linux or Windows kernel version
	GURU_PI_NUMCORES		//!< The number of available CPU cores
} GURU_PLATFORMINFO;

//! GURU_GetPlatformInfo can be used to request information about
//! the OS & platform from the runtime.
//! Unsupported enum values should return NULL.
//! @param eInfoType The type of information the caller wants to obtain
//! @return a zero terminated ASCII string containing the requested information or NULL.
const char *GURU_GetPlatformInfo(GURU_PLATFORMINFO eInfoType);

///////////////////////////////////////////////////////
// Tasks

#define TASK_HANDLE HANDLE
#define INVALID_TASK_HANDLE INVALID_HANDLE_VALUE

//! GURU_CreateTask create a new task and starts it.
//! @param pFunc Entry point of the new task
//! @param Prio Priority of the new task
//! @param dwStackSize Minimum stack size that has to be allocated for this task in bytes
//! @param pArgument User data pointer that will be passed to the task entry point
//! @param pszName Name of the new task. The memory this references needs to remain valid while the task handle is valid.
//! @return A handle to the new task or INVALID_TASK_HANDLE in case of an error.
//!         The numerical value of INVALID_TASK_HANDLE is platform dependent.
TASK_HANDLE GURU_CreateTaskNamed(TASKSTARTFUNC pFunc,TASK_PRIORITY Prio, DWORD dwStackSize, void *pArgument, const char *pszName);
#define GURU_CreateTask(pFunc,Prio,dwStackSize,pArgument) GURU_CreateTaskNamed(pFunc,Prio,dwStackSize,pArgument, #pFunc )

//! Returns a handle to the currently running task
//! @return Handle to the currently running task
TASK_HANDLE GURU_ThisTask(void);

// Suspends/Resumes a task, returns TRUE on success
#define GURU_SuspendTask(hTask) SuspendThread(hTask)
#define GURU_ResumeTask(hTask) ResumeThread(hTask)

// Changes a task priority, returns TRUE on success
#define GURU_SetPriority(hTask,Prio) SetThreadPriority(hTask,Prio);

//! GURU_Delay delays execution of the current task for a number of clock ticks
//! @param dwTicks Number of ticks
void GURU_Delay(DWORD dwTicks);

//! GURU_DeleteTask frees all resources associated with a task (handle).
//! This is NOT a task kill function. It should only be called by "another" thread
//! to delete the task handle of a task that has already terminated.
void GURU_DeleteTask(TASK_HANDLE hTask);


///////////////////////////////////////////////////////
// Semaphores


#define SEMAPHORE_HANDLE HANDLE
#define INVALID_SEMAPHORE_HANDLE INVALID_HANDLE_VALUE
typedef DWORD SEMAPHORE_RESULT;
#define GURU_SEMAPHORE_WAIT_INFINITE	INFINITE
#define GURU_SEMAPHORE_TIMEOUT			WAIT_TIMEOUT
#define GURU_SEMAPHORE_OK				WAIT_OBJECT_0
#define GURU_SEMAPHORE_UNAVAILABLE		WAIT_FAILED

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
//!         - GURU_SEMAPHORE_TIMEOUT if a timeout occurred.
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
//! The behavior of tasks that are currently waiting for the semaphore is undefined.
//! @param hSema A valid semaphore handle
void GURU_SemaphoreDelete(SEMAPHORE_HANDLE hSema);


///////////////////////////////////////////////////////
// Mutexes
#define MUTEX_HANDLE CRITICAL_SECTION

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

typedef struct {
	HANDLE hMutex;
	HANDLE hSema;
	DWORD dwMaxCount;
	DWORD dwMsgNum;
	DWORD *pData;
} X86QUEUE;

#define QUEUE_HANDLE X86QUEUE*
#define INVALID_QUEUE_HANDLE		NULL
#define GURU_QUEUE_WAIT_INFINITE	INFINITE
#define GURU_QUEUE_TIMEOUT			WAIT_TIMEOUT
#define GURU_QUEUE_EMPTY			-2
#define GURU_QUEUE_FULL				-3
#define GURU_QUEUE_OK				WAIT_OBJECT_0

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
int GURU_QueueRecv(QUEUE_HANDLE hQueue, DWORD msg[4], DWORD dwWaitTicks);


//! GURU_QueueTryRecv tries to receive a message from a queue.
//! @param hQueue A valid queue handle
//! @param msg The 4 DWORDs are copied into the msg buffer if the call succeeds.
//! @return The numerical values of these constants are platform dependent:
//!         - GURU_QUEUE_OK A message arrived and has been placed into the msg buffer
//!         - GURU_QUEUE_EMPTY The queue is empty
int GURU_QueueTryRecv(QUEUE_HANDLE hQueue,DWORD msg[4]);

//! GURU_QueueSend sends a message to a queue.
//! @param hQueue A valid queue handle
//! @param msg The message to send. The 4 DWORDs are copied into the QUEUE if the call succeeds.
//! @return The numerical values of these constants are platform dependent:
//!			- GURU_QUEUE_OK The message was sent successfully
//!         - GURU_QUEUE_FULL The queue is full. The message was not sent.
int GURU_QueueSend(QUEUE_HANDLE hQueue, const DWORD msg[4]);

//! GURU_QueueDelete deletes a queue that has been allocated using GURU_CreateQueue().
//! No tasks should be waiting in GURU_QueueRecv for the handle when this function is called.
//! Otherwise the behavior is undefined.
void GURU_QueueDelete(QUEUE_HANDLE hQueue);

///////////////////////////////////////////////////////
// System

//! GURU_Reset should cause a hardware reset of the system.
//! On the PC platform, it terminates the main executable.
void GURU_Reset(void);

#define GURU_STANDBY_INFINITE 0xFFFFFFFE

//! GURU_EnterStandby switches the hardware into a power-saving
//! standby mode. Only an infrared power-up command and expiration
//! of the wakeup timer should bring the box out of this standby mode.
//! This function call should not return.
//! @param dwWakeupSecs The number of seconds after which the system restores power and reboots or GURU_STANDBY_INFINITE.
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
DWORD GURU_GetTickCount(void);

//! The macro MSECSPERTICK defines the number of milliseconds that pass per clock tick
//! The definition that may appear in this documentation is only for illustration purposes.
#define MSECSPERTICK		(1)

//! The macro MSTOTICKS is used to convert milliseconds into clock ticks
#define MSTOTICKS(ms)		((ms)/MSECSPERTICK)

//! The macro TICKSTOMS is used to convert clock ticks into milliseconds
#define TICKSTOMS(ticks)	((ticks)*MSECSPERTICK)

//! GURU_GetKernelCMDLine returns the commandline of the linux kernel (or application) on platforms where such a commandline exists
//! @return The commandline as a zero-terminated string. The returned buffer is static, probably initialized on the first call and does not need to be freed by anyone.
//! The return value is NULL or an empty string on platforms not supporting this feature.
const char *GURU_GetKernelCMDLine(void);

//! GURU_SetOSTime sets the supplied time as system time for the operating system.
//! The OS time influences timestamps on files written to attached media devices.
//! @param t The time to set as new system time.
void GURU_SetOSTime(time_t t);

#ifdef __cplusplus
}
#endif

#endif // __HAL_OS_H
