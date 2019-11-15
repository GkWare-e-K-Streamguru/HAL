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
#include "all.h"


#include <sys/mount.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>



void GURU_Init(void)
{

}

const char *GURU_GetPlatformInfo(GURU_PLATFORMINFO eInfoType)
{
	static char pszCompilerString[30];
	static char pszKernelString[120];
	static char pszNumCores[100];
	static struct utsname un;
	int ret;

	ret = uname(&un);
	assert(ret==0);

	//UART_printf("system name = %s\n", un.sysname);
	//UART_printf("node name   = %s\n", un.nodename);
	//UART_printf("release     = %s\n", un.release);
	//UART_printf("version     = %s\n", un.version);
	//UART_printf("machine     = %s\n", un.machine);

	switch(eInfoType)
	{
	case GURU_PI_CHIPTYPE:
#ifdef __i386__
		return "x86";
#elif __amd64__
		return "x64";
#elif __mips__
		return "Mips";
#else
#error
#endif
		return un.machine;
		
	case GURU_PI_BOARDREV:
		return PLATFORM_NAME;
	case GURU_PI_COMPILER:
		sprintf(pszCompilerString,"gcc %d.%d.%d",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);
		return pszCompilerString;
	case GURU_PI_BUILDTYPE:
#ifdef _DEBUG
		return "Debug";
#else
		return "Release";
#endif
	case GURU_PI_KERNELVER:
		sprintf(pszKernelString,"%s %s (%s)",un.sysname,un.release,un.version);
		return pszKernelString;		
	case GURU_PI_NUMCORES:
		sprintf(pszNumCores, "%d", (int)sysconf(_SC_NPROCESSORS_ONLN));
		return pszNumCores;
	default:
		assert(0);
		return NULL;
	}
}


DWORD GURU_GetTickCount(void)
{
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	tm.tv_sec*=1000;
	tm.tv_nsec/=1000000;
	return tm.tv_sec+tm.tv_nsec;
}

typedef struct {
	TASKSTARTFUNC pFunc;
	void *pArg;
} THREADSTARTINFO;

static void* tempstartfunc(void *pArg)
{
	THREADSTARTINFO *pTSI = (THREADSTARTINFO *)pArg;
	TASKSTARTFUNC pFunc = pTSI->pFunc;
	void *pArg2 = pTSI->pArg;
	free(pTSI);

	// 	int type;
	// 	struct sched_param sch_params;
	// 	pthread_getschedparam(pthread_self(), &type, &sch_params);
	// 	UART_printf("task priority: %d\n", sch_params.sched_priority);

	pFunc(pArg2);
	
	pthread_exit(NULL);
	return NULL;
}

TASK_HANDLE GURU_CreateTaskNamed(TASKSTARTFUNC pFunc,TASK_PRIORITY Prio, DWORD dwStackSize, void *pArgument, const char *pszName)
{
	pthread_attr_t attr;
	pthread_t pthread_id;
	struct sched_param sch_params;
	THREADSTARTINFO *pTSI = (THREADSTARTINFO *)malloc(sizeof(THREADSTARTINFO));
	int ret;
	
	assert(pTSI);
	if(!pTSI) {
		return INVALID_TASK_HANDLE;
	}
	pTSI->pArg = pArgument;
	pTSI->pFunc = pFunc;
	
	pthread_attr_init(&attr);
	memset(&sch_params, 0x00, sizeof(sch_params));
 	sch_params.sched_priority = Prio;
#ifdef PLATFORM_MSTAR // this only works as root
 	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	assert(ret == 0);
#endif
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	assert(ret == 0);
    ret = pthread_attr_setschedparam(&attr,&sch_params);
	assert(ret == 0);
 	ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	assert(ret == 0);
 	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	assert(ret == 0);
	
	errno = 0;
	ret = pthread_create(&pthread_id , &attr, tempstartfunc, pTSI);
	if(ret != 0) {
		UART_printf("pthread_create failed with ret %d errno %d %d\r\n",ret,errno,ENOMEM);
		pthread_attr_destroy(&attr);
		assert(0);
		return INVALID_TASK_HANDLE;
	}
	pthread_attr_destroy(&attr);
	if(pszName) {
		// there is a 16 character limit (including the terminating zero)
		char pszName2[17];
		if(strlen(pszName)>=15) {
			memcpy(pszName2,pszName,15);
			pszName2[15]='\0';
			pszName = pszName2;
		}
		// assert(strlen(pszName)<16);

		pthread_setname_np(pthread_id,pszName);

	}
	pthread_detach(pthread_id);
	return pthread_id;
}

TASK_HANDLE GURU_ThisTask(void)
{
	return pthread_self();
}

void GURU_DeleteTask(TASK_HANDLE hTask)
{
	// neither necessary nor possible after pthread_detach
	// pthread_join(hTask, NULL);
}

void GURU_Delay(DWORD dwTicks)
{
	usleep(dwTicks*1000);
}

SEMAPHORE_HANDLE GURU_CreateSemaphore(DWORD dwInitialCount)
{
	sem_t *pSem;
	pSem = (sem_t *)malloc(sizeof(sem_t));
	if(!pSem)
		return NULL;
	sem_init(pSem,0,dwInitialCount);
	return pSem;
}

SEMAPHORE_RESULT GURU_SemaphoreWait(SEMAPHORE_HANDLE hSema, DWORD dwWaitTicks)
{
	struct timespec tm;
	int ret;
	if(dwWaitTicks == GURU_SEMAPHORE_WAIT_INFINITE)
	{
		if(sem_wait(hSema) == 0)
			return GURU_SEMAPHORE_OK;
		else
			return GURU_SEMAPHORE_UNAVAILABLE;
	}
	
	clock_gettime(CLOCK_REALTIME, &tm);
	tm.tv_nsec += (dwWaitTicks%1000) * 1000000;
	tm.tv_sec += dwWaitTicks/1000;

	while(tm.tv_nsec > 1000000000) {
		tm.tv_nsec -= 1000000000;
		tm.tv_sec++;
	}
	errno = 0;
	ret = sem_timedwait(hSema,&tm);
	switch(ret)
	{
	case 0:
		return GURU_SEMAPHORE_OK;
	default:
		switch(errno)
		{
		case EINVAL:
			UART_printf("GURU_SemaphoreWait EINVAL\r\n");
			break;
		case ETIMEDOUT:
			return GURU_SEMAPHORE_TIMEOUT;
		case EDEADLK:
			UART_printf("GURU_SemaphoreWait EDEADLK\r\n");
			break;
		case EINTR:
			UART_printf("GURU_SemaphoreWait EINTR\r\n");
			break;
		default:
			UART_printf("GURU_SemaphoreWait errno=%d\r\n",errno);
			break;
		}
		
		return GURU_SEMAPHORE_UNAVAILABLE;
	}
}

SEMAPHORE_RESULT GURU_SemaphoreTry(SEMAPHORE_HANDLE hSema)
{
	int ret;
	
	errno = 0;
	ret = sem_trywait(hSema);
	switch(ret)
	{
	case 0:
		return GURU_SEMAPHORE_OK;
	default:
		switch(errno)
		{
		case EINVAL:
			UART_printf("GURU_SemaphoreWait EINVAL\r\n");
			break;
		case ETIMEDOUT:
			return GURU_SEMAPHORE_TIMEOUT;
		case EDEADLK:
			UART_printf("GURU_SemaphoreWait EDEADLK\r\n");
			break;
		case EINTR:
			UART_printf("GURU_SemaphoreWait EINTR\r\n");
			break;
		default:
			UART_printf("GURU_SemaphoreWait errno=%d\r\n",errno);
			break;
		}
		
		return GURU_SEMAPHORE_UNAVAILABLE;
	}
}


void GURU_SemaphoreRelease(SEMAPHORE_HANDLE hSema)
{
	sem_post(hSema);
}

void GURU_SemaphoreDelete(SEMAPHORE_HANDLE hSema)
{
	sem_destroy(hSema);
}

BOOL GURU_CreateMutex(MUTEX_HANDLE *pMutex)
{
	pthread_mutex_t m = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	*pMutex = m;
	return TRUE;
}

void GURU_Mutex_Lock(MUTEX_HANDLE *pMutex)
{
	pthread_mutex_lock( pMutex );
}

void GURU_Mutex_Unlock(MUTEX_HANDLE *pMutex)
{
	pthread_mutex_unlock( pMutex );
}

void GURU_Mutex_Delete(MUTEX_HANDLE *pMutex)
{
	UNUSED_PARAMETER(pMutex);
}


QUEUE_HANDLE GURU_CreateQueue(DWORD dwMaxCount)
{
	LINUXQUEUE *pQueue;
	pQueue = (LINUXQUEUE *)malloc(sizeof(LINUXQUEUE));
	assert(pQueue);
	if(!pQueue)
		return INVALID_QUEUE_HANDLE;
	pQueue->hMsgSema = GURU_CreateSemaphore(0);
	pQueue->hLock = GURU_CreateSemaphore(1);
	pQueue->nMessages = 0;
	pQueue->dwMaxCount = dwMaxCount;
	pQueue->pData = malloc(dwMaxCount*4*sizeof(DWORD));
	assert(pQueue->pData);
	if(!pQueue->pData) {
		free(pQueue);
		return INVALID_QUEUE_HANDLE;
	}
	return pQueue;
}

int GURU_QueueRecv(QUEUE_HANDLE hQueue, DWORD msg[4], DWORD dwWaitTicks)
{
	assert(hQueue);
	assert(hQueue->hLock);
	assert(hQueue->hMsgSema);
	switch(GURU_SemaphoreWait(hQueue->hMsgSema, dwWaitTicks)) 
	{
	case GURU_SEMAPHORE_OK:
		break;
	case GURU_SEMAPHORE_TIMEOUT:
		return GURU_QUEUE_TIMEOUT;
	default:
		return GURU_QUEUE_EMPTY;
	}

	GURU_SemaphoreWait(hQueue->hLock, GURU_SEMAPHORE_WAIT_INFINITE);
	assert(hQueue->nMessages);
	msg[0] = hQueue->pData[0];
	msg[1] = hQueue->pData[1];
	msg[2] = hQueue->pData[2];
	msg[3] = hQueue->pData[3];
	hQueue->nMessages--;
	memmove(hQueue->pData,&hQueue->pData[4],4*hQueue->nMessages*sizeof(DWORD));
	//UART_printf("Received (rem %d) %d %d %d %d\r\n",hQueue->nMessages,msg[0],msg[1],msg[2],msg[3]);
	GURU_SemaphoreRelease(hQueue->hLock);
	return GURU_QUEUE_OK;
}

int GURU_QueueTryRecv(QUEUE_HANDLE hQueue,DWORD msg[4])
{
	return GURU_QueueRecv(hQueue, msg, 0);	
}

int GURU_QueueSend(QUEUE_HANDLE hQueue, const DWORD msg[4])
{
	GURU_SemaphoreWait(hQueue->hLock, GURU_SEMAPHORE_WAIT_INFINITE);
	if(hQueue->nMessages == hQueue->dwMaxCount) {
		GURU_SemaphoreRelease(hQueue->hLock);
		return GURU_QUEUE_FULL;
	}
	//UART_printf("Sending into queue with %d msgs %d %d %d %d\r\n",hQueue->nMessages,msg[0],msg[1],msg[2],msg[3]);
	hQueue->pData[4*hQueue->nMessages+0] = msg[0];
	hQueue->pData[4*hQueue->nMessages+1] = msg[1];
	hQueue->pData[4*hQueue->nMessages+2] = msg[2];
	hQueue->pData[4*hQueue->nMessages+3] = msg[3];
	hQueue->nMessages++;
	GURU_SemaphoreRelease(hQueue->hLock);

	GURU_SemaphoreRelease(hQueue->hMsgSema);
	return GURU_QUEUE_OK;
}

void GURU_QueueDelete(QUEUE_HANDLE hQueue)
{
	GURU_SemaphoreDelete(hQueue->hMsgSema);
	GURU_SemaphoreDelete(hQueue->hLock);
	free(hQueue->pData);
	memset(hQueue,0x00,sizeof(LINUXQUEUE));
	free(hQueue);
}

#ifndef PLATFORM_MSTAR
void GURU_EnterHardwareStandby(DWORD dwWakeupSecs)
{

}

WAKEUP_REASON GURU_GetWakeupReason(void)
{
	return WAKEUP_REASON_UNKNOWN;
}
#endif

void GURU_Reset(void)
{
#ifdef PLATFORM_MSTAR
	sync();
	system("reboot");
#endif
}

#define MAX_KERNEL_CMDLINE 1024
static char g_pszKernelCmdLine[MAX_KERNEL_CMDLINE+1] = {'\0'};

const char *GURU_GetKernelCMDLine(void)
{
	if(g_pszKernelCmdLine[0])
		return g_pszKernelCmdLine;
	memset(g_pszKernelCmdLine, 0x00, MAX_KERNEL_CMDLINE+1);
	FILE_HANDLE hCmdLine = FSYS_Open("/proc/cmdline", FALSE);
	assert(hCmdLine != INVALID_FILE_HANDLE);
	if(hCmdLine) {
		int nRead;
		nRead = FSYS_Read(hCmdLine, g_pszKernelCmdLine, MAX_KERNEL_CMDLINE);
		if(nRead>0)
			g_pszKernelCmdLine[nRead] = '\0';
		//PrintHexDump(g_pszKernelCmdLine, strlen(g_pszKernelCmdLine));
		FSYS_Close(hCmdLine);
	}
	return g_pszKernelCmdLine;
}

void GURU_SetOSTime(time_t t)
{
	struct timeval tv;
	tv.tv_sec = t;
	tv.tv_usec = 0;
	settimeofday(&tv, NULL);
}
