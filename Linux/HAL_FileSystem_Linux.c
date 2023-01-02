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
#include "../HAL_FileSystem.h"

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

// #define FSYS_DEBUG_ENABLE

#ifdef FSYS_DEBUG_ENABLE
#define FSYS_DEBUG(x) UART_printf x
FAILRELEASEBUILD
#else
#define FSYS_DEBUG(x)
#endif

#define O_BINARY 0


FILE_HANDLE FSYS_Open(const char *pszFName, FSYS_OPEN_MODE eOpenMode)
{
	int hFile;
	assert(pszFName);

	FSYS_DEBUG(("FSYS_Open %s WriteMode%d\r\n", pszFName, eOpenMode));


	switch(eOpenMode)
	{
	case FSYS_READONLY:
		hFile = open(pszFName, O_RDONLY | O_BINARY);
		break;
	case FSYS_READWRITE:
		hFile = open(pszFName, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		break;
	case FSYS_APPEND:
		hFile = open(pszFName, O_RDWR | O_CREAT | O_APPEND | O_BINARY, S_IREAD | S_IWRITE);
		break;
	default:
		assert(0);
		return INVALID_FILE_HANDLE;
	}

	if(hFile == -1)
		return INVALID_FILE_HANDLE;

	FSYS_DEBUG(("FSYS_Open %s WriteMode%d = handle %d\r\n", pszFName, eOpenMode, (int)hFile));
	return (FILE_HANDLE)hFile;
}

int FSYS_Read(FILE_HANDLE hFile, void *pBuf, DWORD dwLen)
{
	assert(hFile);
	assert(pBuf);

	FSYS_DEBUG(("FSYS_Read Handle%d Len%d\r\n",(int)hFile, (int)dwLen));
#ifdef _DEBUG
	memset(pBuf, 0xDD, dwLen);
#endif
	return read((int)hFile,pBuf,dwLen);
}

int FSYS_Write(FILE_HANDLE hFile, const void *pBuf, DWORD dwLen)
{
	assert(hFile);
	assert(pBuf);
	FSYS_DEBUG(("FSYS_Write Handle%d Len%d\r\n",(int)hFile, (int)dwLen));
	return write((int)hFile, pBuf, dwLen);
}

FILESIZE_TYPE FSYS_GetFileSize(FILE_HANDLE hFile)
{
	FILESIZE_TYPE dwLen;
	struct stat s;
	int ret;
	assert(hFile);
	ret = fstat((int)hFile, &s);
	assert(ret == 0);
	if(ret != 0)
		return 0;
	dwLen = s.st_size;
	FSYS_DEBUG(("FSYS_GetFileSize Handle%d = Len%d\r\n",(int)hFile, (int)dwLen));
	return dwLen;
}

int FSYS_SetFilePos(FILE_HANDLE hFile, int nPos, FSYS_FILEPOS_MODE eMode)
{
	FSYS_DEBUG(("FSYS_SetFilePos Handle%d Pos%d Mode%d\r\n", (int)hFile, nPos, (int)eMode));

	switch(eMode)
	{
	case FSYS_FP_START:
		return lseek((int)hFile, nPos, SEEK_SET);
	case FSYS_FP_CUR:
		return lseek((int)hFile, nPos, SEEK_CUR);
	case FSYS_FP_END:
		return lseek((int)hFile, nPos, SEEK_END);
	default:
		assert(0);
	}
	return -1;
}

void FSYS_Close(FILE_HANDLE hFile)
{
	FSYS_DEBUG(("FSYS_Close Handle%d\r\n", (int)hFile));

	syncfs((int)hFile);

	close((int)hFile);
}

BOOL FSYS_DeleteFile(const char *pszFName)
{
	FSYS_DEBUG(("FSYS_DeleteFile %s\r\n", pszFName));

	if(unlink(pszFName)==0)
		return TRUE;
	else
		return FALSE;
}

BOOL FSYS_CreateDirectory(const char *pszDirectory)
{
	FSYS_DEBUG(("FSYS_CreateDirectory %s\r\n", pszDirectory));
	
    // Several "man-pages" document the return value of mkdir:
    //     Upon successful completion, these functions shall return 0. Otherwise, these functions shall return -1
    //     and set errno to indicate the error.
    //     If -1 is returned, no directory shall be created.

	if(mkdir(pszDirectory, 0777)==0)
        return TRUE;
	return FALSE;
}


BOOL FSYS_DeleteDirectory(const char *pszDirectory)
{
	FSYS_DEBUG(("FSYS_DeleteDirectory %s\r\n", pszDirectory));
	
    // Several "man-pages" document the return value of mkdir:
    //     Upon successful completion, the function rmdir() shall return 0. Otherwise, -1 shall be returned,
    //     and errno set to indicate the error.
    //     If -1 is returned, the named directory shall not be changed.
	if(rmdir(pszDirectory)==0)
		return TRUE;
	return FALSE;
}

typedef struct {
	DIR *hFind;
	char pszDir[MAX_PATH+1];
} FINDHANDLE;

FS_FINDFILE_HANDLE FSYS_FindDirContents(const char *pszDirectory, FSYS_FINDFILE_INFO *pInfo)
{
	FSYS_DEBUG(("FSYS_FindDirContents %s\r\n", pszDirectory));
	assert(pszDirectory);
	assert(pInfo);
	assert(strlen(pszDirectory)<=MAX_PATH);
	if(!pszDirectory || strlen(pszDirectory)>=MAX_PATH)
		return INVALID_FS_FINDFILE_HANDLE;

	FINDHANDLE *pFind = malloc(sizeof(FINDHANDLE));
	assert(pFind);
	if(!pFind)
		return INVALID_FS_FINDFILE_HANDLE;

	strcpy(pFind->pszDir, pszDirectory);
#ifdef _DEBUG
	memset(pInfo, 0xDD, sizeof(FSYS_FINDFILE_INFO));
#endif

	if(!strlen(pFind->pszDir) || pFind->pszDir[strlen(pFind->pszDir)-1]!='/')
		strcat(pFind->pszDir,"/");

	pFind->hFind = opendir(pszDirectory);
	if(pFind->hFind == NULL) {
		free(pFind);
		return INVALID_FS_FINDFILE_HANDLE;
	}

	if(!FSYS_FindNextDirEntry(pFind,pInfo)) {
		FSYS_FindClose(pFind);
		return NULL;
	}
	return pFind;
}


BOOL FSYS_FindNextDirEntry(FS_FINDFILE_HANDLE hFind, FSYS_FINDFILE_INFO *pInfo)
{
	FINDHANDLE *pFind = hFind;	
	struct dirent *de;
	//FSYS_DEBUG(("FSYS_FindNextDirEntry Handle%08X\r\n", (int)hFind));

	assert(hFind);
	assert(pInfo);
#ifdef _DEBUG
	memset(pInfo, 0xDD, sizeof(FSYS_FINDFILE_INFO));
#endif

	do {
		de = readdir(pFind->hFind);
		if(!de)
			return FALSE;
	} while((strcmp(de->d_name,".")==0) || (strcmp(de->d_name,"..")==0));
	pInfo->dwFileSize = 0;
	pInfo->fIsDirectory = FALSE;
	strncpy(pInfo->pszFName, de->d_name,MAX_PATH-1);
	pInfo->pszFName[MAX_PATH-1] = '\0';


// 		struct stat {
// 			dev_t     st_dev;     /* ID of device containing file */
// 			ino_t     st_ino;     /* inode number */
// 			mode_t    st_mode;    /* protection */
// 			nlink_t   st_nlink;   /* number of hard links */
// 			uid_t     st_uid;     /* user ID of owner */
// 			gid_t     st_gid;     /* group ID of owner */
// 			dev_t     st_rdev;    /* device ID (if special file) */
// 			off_t     st_size;    /* total size, in bytes */
// 			blksize_t st_blksize; /* blocksize for file system I/O */
// 			blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
// 			time_t    st_atime;   /* time of last access */
// 			time_t    st_mtime;   /* time of last modification */
// 			time_t    st_ctime;   /* time of last status change */
// 		};
	struct stat s;

	char pszFullPath[2*MAX_PATH];
	assert(strlen(pFind->pszDir)+strlen(pInfo->pszFName) <= MAX_PATH);
	if(strlen(pFind->pszDir)+strlen(pInfo->pszFName) > MAX_PATH)
		return FALSE;
	sprintf(pszFullPath,"%s%s",pFind->pszDir,pInfo->pszFName);

	if(stat(pszFullPath,&s)!=0)
	{
		FSYS_DEBUG(("stat failed for %s with %d\r\n",pszFullPath,errno));
		assert(0);
		return FALSE;
	}
	if(S_ISDIR(s.st_mode))
		pInfo->fIsDirectory = TRUE;
	pInfo->dwFileSize = s.st_size;
	pInfo->tCreationTime = s.st_mtime;
	return TRUE;
}

void FSYS_FindClose(FS_FINDFILE_HANDLE hFind)
{
	FINDHANDLE *pFind = hFind;	
	
	assert(hFind);
	FSYS_DEBUG(("FSYS_FindClose Handle%08X\r\n", (int)hFind));
	closedir(pFind->hFind);
	free(pFind);
}
