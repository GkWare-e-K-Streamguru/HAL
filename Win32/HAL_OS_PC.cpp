// ********************************************************
//
//   Author/Copyright	Gero Kuehn / GkWare e.K.
//						Hatzper Strasse 172B
//						D-45149 Essen, Germany
//						Tel: +49 174 520 8026
//						Email: support@gkware.com
//						Web: http://www.gkware.com
//
//
// ********************************************************
#include <assert.h>
#include "all.h"


typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

static void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	{
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
	}
	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

static DWORD g_dwTlsIndex = TLS_OUT_OF_INDEXES;
static SEMAPHORE_HANDLE g_hSemaTaskStart = INVALID_SEMAPHORE_HANDLE;
static QUEUE_HANDLE g_hQueueTaskStart = INVALID_QUEUE_HANDLE;


typedef struct  
{
	void *pArgument;
	TASK_PRIORITY ePrio;
	TASKSTARTFUNC pFunc;
}TASKSTARTER_ARG;

static int GetWin32Priority(TASK_PRIORITY ePrio)
{
	switch(ePrio)
	{
	case TASK_PRIO_IDLE:
		return THREAD_PRIORITY_IDLE;
	case TASK_PRIO_LOWEST:
		return THREAD_PRIORITY_LOWEST;
	case TASK_PRIO_LOW:
		return THREAD_PRIORITY_BELOW_NORMAL;
	case TASK_PRIO_BELOW_NORMAL:
		return THREAD_PRIORITY_BELOW_NORMAL; // duplicate
	case TASK_PRIO_NORMAL:
		return THREAD_PRIORITY_NORMAL;
	case TASK_PRIO_ABOVE_NORMAL:
		return THREAD_PRIORITY_ABOVE_NORMAL;
	case TASK_PRIO_HIGH:
		return THREAD_PRIORITY_HIGHEST;
	case TASK_PRIO_HIGHEST:
		return THREAD_PRIORITY_TIME_CRITICAL;
	default:
		assert(0);
		return THREAD_PRIORITY_NORMAL;
	}	
}

static void GURU_TaskStarter(void *pArgument)
{
	TASKSTARTER_ARG *pThreadStarterArg = (TASKSTARTER_ARG*)pArgument;
	DWORD TaskStartMsg[4];
	pArgument = pThreadStarterArg->pArgument;
	TASKSTARTFUNC pFunc = pThreadStarterArg->pFunc;
	SetThreadPriority(GetCurrentThread(), GetWin32Priority(pThreadStarterArg->ePrio));
	free(pThreadStarterArg);

	if(g_dwTlsIndex != TLS_OUT_OF_INDEXES) {
		GURU_QueueRecv(g_hQueueTaskStart, TaskStartMsg, GURU_QUEUE_WAIT_INFINITE);
		TlsSetValue(g_dwTlsIndex, (void*)TaskStartMsg[0]); // Store the handle into Tls
		GURU_SemaphoreRelease(g_hSemaTaskStart);
	}
	pFunc(pArgument);
	
}

TASK_HANDLE GURU_CreateTaskNamed(TASKSTARTFUNC pFunc,TASK_PRIORITY Prio, DWORD dwStackSize, void *pArgument, const char *pszName)
{
	DWORD hThread;
	TASKSTARTER_ARG *pThreadStarterArg = (TASKSTARTER_ARG*)malloc(sizeof(TASKSTARTER_ARG));
	DWORD TaskStartMsg[4];

	UNUSED_PARAMETER(dwStackSize);

	assert(pThreadStarterArg);
	if(!pThreadStarterArg) { 
		return INVALID_TASK_HANDLE;
	}
	GetWin32Priority(Prio);

	pThreadStarterArg->pFunc = pFunc;
	pThreadStarterArg->pArgument = pArgument;
	pThreadStarterArg->ePrio = Prio;
	if(g_dwTlsIndex != TLS_OUT_OF_INDEXES) {
		GURU_SemaphoreWait(g_hSemaTaskStart, GURU_SEMAPHORE_WAIT_INFINITE);
	}
	hThread = _beginthread(GURU_TaskStarter,0,pThreadStarterArg);
	if(hThread == -1) {
		GURU_SemaphoreRelease(g_hSemaTaskStart);
		free(pThreadStarterArg);
		return INVALID_TASK_HANDLE;
	}
	if(g_dwTlsIndex != TLS_OUT_OF_INDEXES) {
		TaskStartMsg[0] = (DWORD)hThread;
		GURU_QueueSend(g_hQueueTaskStart, TaskStartMsg);
	}
	SetThreadName(hThread,pszName);

	return (TASK_HANDLE)hThread;
}

// Returns a handle to the currently running task, it should be stored in Tls
TASK_HANDLE GURU_ThisTask(void)
{
	if(g_dwTlsIndex != TLS_OUT_OF_INDEXES) {
		return (TASK_HANDLE)TlsGetValue(g_dwTlsIndex); // Return the stored thread handle from Tls
	}
	return GetCurrentThread();
}

void GURU_DeleteTask(TASK_HANDLE hTask)
{
	
}

void GURU_Delay(DWORD dwTicks)
{
	Sleep(dwTicks);
}

#pragma warning( push )
#pragma warning( disable : 4702 ) // unreachable code
const char *GURU_GetPlatformInfo(GURU_PLATFORMINFO eInfoType)
{
	switch(eInfoType)
	{
	case GURU_PI_CHIPTYPE:
#ifdef _M_AMD64
		return "x64";
#else
		return "x86";
#endif
	case GURU_PI_BOARDREV:
		return NULL;
	case GURU_PI_COMPILER:
#if _MSC_VER == 1200
		return "Visual C++ 6";
#endif
#if _MSC_VER == 1300
		return "Visual Studio 2003";
#endif
#if _MSC_VER == 1400
		return "Visual Studio 2005";
#endif
#if _MSC_VER == 1500
		return "Visual Studio 2008";
#endif
#if _MSC_VER == 1600
		return "Visual Studio 2010";
#endif
#if _MSC_VER == 1700
		return "Visual Studio 2012";
#endif
#if _MSC_VER == 1800
		return "Visual Studio 2013";
#endif
#if _MSC_VER == 1900
		return "Visual Studio 2015";
#endif
#if _MSC_VER == 1910
		return "Visual Studio 2017";
#endif
		return "unknown";
	case GURU_PI_BUILDTYPE:
#ifdef _DEBUG
		return "Debug";
#else
		return "Release";
#endif
	case GURU_PI_KERNELVER:
		OSVERSIONINFO osvi;
		static char pszText[100];
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		sprintf(pszText,"Windows %d.%d(%d)",osvi.dwMajorVersion,osvi.dwMinorVersion,osvi.dwBuildNumber);
		return pszText;
	case GURU_PI_NUMCORES:
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		static char pszNumCores[100];
		sprintf(pszNumCores, "%d", si.dwNumberOfProcessors);
		return pszNumCores;
	default:
		assert(0);
		return NULL;
	}
}
#pragma warning( pop ) 

void GURU_Init(void)
{
	if((g_dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
		assert(0);
	}
	if( (g_dwTlsIndex != TLS_OUT_OF_INDEXES) && ((g_hSemaTaskStart = GURU_CreateSemaphore(1)) == INVALID_SEMAPHORE_HANDLE) ) {
		assert(0);
		TlsFree(g_dwTlsIndex);
		g_dwTlsIndex = TLS_OUT_OF_INDEXES;
	}
	if( (g_dwTlsIndex != TLS_OUT_OF_INDEXES) && ((g_hQueueTaskStart = GURU_CreateQueue(1)) == INVALID_QUEUE_HANDLE) ) {
		assert(0);
		TlsFree(g_dwTlsIndex);
		GURU_SemaphoreDelete(g_hSemaTaskStart);
		g_dwTlsIndex = TLS_OUT_OF_INDEXES;
	}
}


SEMAPHORE_HANDLE GURU_CreateSemaphore(DWORD dwInitialCount)
{
	return CreateSemaphore(NULL,dwInitialCount,0xFFFF,NULL);
}

SEMAPHORE_RESULT GURU_SemaphoreWait(SEMAPHORE_HANDLE hSema, DWORD dwWaitTicks)
{
	return WaitForSingleObject(hSema,dwWaitTicks);
}

SEMAPHORE_RESULT GURU_SemaphoreTry(SEMAPHORE_HANDLE hSema)
{
	return WaitForSingleObject(hSema,0);
}

void GURU_SemaphoreRelease(SEMAPHORE_HANDLE hSema)
{
	ReleaseSemaphore(hSema,1,NULL);
}

void GURU_SemaphoreDelete(SEMAPHORE_HANDLE hSema)
{
	CloseHandle(hSema);
}

BOOL GURU_CreateMutex(MUTEX_HANDLE *pMutex)
{
	assert(pMutex);
	return InitializeCriticalSectionAndSpinCount(pMutex, 0x80000000);
}

void GURU_Mutex_Lock(MUTEX_HANDLE *pMutex)
{
	assert(pMutex);
	EnterCriticalSection(pMutex);
}

void GURU_Mutex_Unlock(MUTEX_HANDLE *pMutex)
{
	assert(pMutex);
	LeaveCriticalSection(pMutex);
}

void GURU_Mutex_Delete(MUTEX_HANDLE *pMutex)
{
	assert(pMutex);
	DeleteCriticalSection(pMutex);
}


QUEUE_HANDLE GURU_CreateQueue(DWORD dwMaxCount)
{
	X86QUEUE *pQueue;
	pQueue = (X86QUEUE *)malloc(sizeof(X86QUEUE));
	assert(pQueue);
	if(!pQueue)
		return INVALID_QUEUE_HANDLE;
	pQueue->hMutex = CreateMutex(NULL,FALSE,NULL);
	assert(pQueue->hMutex != NULL);
	pQueue->hSema = CreateSemaphore(NULL,0,0xFFFFFFF,NULL);
	assert(pQueue->hSema != NULL);
	pQueue->pData = (DWORD*)malloc(dwMaxCount*4*4);
	assert(pQueue->pData);
	if(!pQueue->pData) {
		CloseHandle(pQueue->hMutex);
		CloseHandle(pQueue->hSema);
		free(pQueue);
		return INVALID_QUEUE_HANDLE;
	}
	pQueue->dwMaxCount = dwMaxCount;
	pQueue->dwMsgNum = 0;
	return pQueue;
}


int GURU_QueueRecv(QUEUE_HANDLE hQueue,DWORD msg[4],DWORD dwWaitTicks)
{
	DWORD dwRet;
	if(!hQueue)
		return GURU_QUEUE_EMPTY;
	dwRet = WaitForSingleObject(hQueue->hSema,dwWaitTicks);
	if(dwRet == WAIT_TIMEOUT)
		return WAIT_TIMEOUT;
	if(dwRet == WAIT_ABANDONED)
		return GURU_QUEUE_EMPTY;
	if(dwRet == WAIT_FAILED)
		return GURU_QUEUE_EMPTY;
	assert(dwRet == WAIT_OBJECT_0);
	dwRet = WaitForSingleObject(hQueue->hMutex,INFINITE);
	if(dwRet == WAIT_ABANDONED)
		return GURU_QUEUE_EMPTY;
	if(dwRet == WAIT_FAILED)
		return GURU_QUEUE_EMPTY;
	assert(dwRet == WAIT_OBJECT_0);
	memcpy(msg,hQueue->pData,4*sizeof(DWORD));
	hQueue->dwMsgNum--;
	if(hQueue->dwMsgNum)
		memmove(hQueue->pData,hQueue->pData+4,4*sizeof(DWORD)*hQueue->dwMsgNum);
	ReleaseMutex(hQueue->hMutex);
	return GURU_QUEUE_OK;
}
int GURU_QueueTryRecv(QUEUE_HANDLE hQueue,DWORD msg[4])
{
	DWORD dwRet;
	if(!hQueue)
		return GURU_QUEUE_EMPTY;
	dwRet = WaitForSingleObject(hQueue->hSema,0);
	if(dwRet == WAIT_TIMEOUT)
		return GURU_QUEUE_EMPTY;
	if(dwRet == WAIT_ABANDONED)
		return GURU_QUEUE_EMPTY;
	if(WaitForSingleObject(hQueue->hMutex,INFINITE) == WAIT_ABANDONED)
		return GURU_QUEUE_EMPTY;
	memcpy(msg,hQueue->pData,4*sizeof(DWORD));
	hQueue->dwMsgNum--;
	if(hQueue->dwMsgNum)
		memmove(hQueue->pData,hQueue->pData+4,4*sizeof(DWORD)*hQueue->dwMsgNum);
	ReleaseMutex(hQueue->hMutex);
	return GURU_QUEUE_OK;
}
int GURU_QueueSend(QUEUE_HANDLE hQueue, const DWORD msg[4])
{
	if(!hQueue)
		return GURU_QUEUE_FULL;
	WaitForSingleObject(hQueue->hMutex,INFINITE);
	if(hQueue->dwMsgNum == hQueue->dwMaxCount) {
		ReleaseMutex(hQueue->hMutex);
		return GURU_QUEUE_FULL;
	}
	memcpy(hQueue->pData+4*hQueue->dwMsgNum,msg,4*sizeof(DWORD));
	hQueue->dwMsgNum++;
	ReleaseSemaphore(hQueue->hSema,1,NULL);
	ReleaseMutex(hQueue->hMutex);
	return GURU_QUEUE_OK;
}

void GURU_QueueDelete(QUEUE_HANDLE hQueue)
{
	CloseHandle(hQueue->hMutex);
	CloseHandle(hQueue->hSema);
	free(hQueue->pData);
	free(hQueue);
}

void GURU_EnterHardwareStandby(DWORD dwWakeupSecs)
{
#ifdef UART_printf
	UART_printf("GURU_EnterHardwareStandby %d\r\n",dwWakeupSecs);
#endif
}

WAKEUP_REASON GURU_GetWakeupReason(void)
{
	return WAKEUP_REASON_UNKNOWN;
}


void GURU_Reset(void) 
{
	TerminateProcess(GetCurrentProcess(),0);
}

DWORD GURU_GetTickCount()
{
	return GetTickCount();
}

void GURU_SetOSTime(time_t t)
{
	// intentionally ignored. We only do this on embedded targets without buffered RTC.
}
