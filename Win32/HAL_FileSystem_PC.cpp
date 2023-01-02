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
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

//#define FSYS_DEBUG_ENABLE

#ifdef FSYS_DEBUG_ENABLE
#define FSYS_DEBUG(x) UART_printf x
FAILRELEASEBUILD
#else
#define FSYS_DEBUG(x)
#endif



FILE_HANDLE FSYS_Open(const char *pszFName, FSYS_OPEN_MODE eOpenMode)
{
	int hFile;
	assert(pszFName);

	FSYS_DEBUG(("FSYS_Open %s WriteMode%d\r\n", pszFName, eOpenMode));

#ifdef HAL_FILESYS_GKTV_FIXPATH
	char pszFName2[MAX_PATH];
	if(!FixAndConvertFName(pszFName,pszFName2))
		return INVALID_FILE_HANDLE;
	pszFName = pszFName2;
#endif

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
	DWORD dwLen;
	assert(hFile);
	dwLen = filelength((int)hFile);
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
	
#ifdef UNICODE
    wchar_t wpszDirectory[MAX_PATH+1];
    swprintf(wpszDirectory, L"%S", pszDirectory);
    if(CreateDirectory(wpszDirectory, NULL)==0)
#else // #ifdef UNICODE
	if(CreateDirectory(pszDirectory, NULL)==0)
#endif // #ifdef UNICODE
		return FALSE;
	return TRUE;
}


BOOL FSYS_DeleteDirectory(const char *pszDirectory)
{
	FSYS_DEBUG(("FSYS_DeleteDirectory %s\r\n", pszDirectory));
	
#ifdef UNICODE
    wchar_t wpszDirectory[MAX_PATH+1];
    swprintf(wpszDirectory, L"%S", pszDirectory);
    if(RemoveDirectory(wpszDirectory)==0)
#else // #ifdef UNICODE
	if(RemoveDirectory(pszDirectory)==0)
#endif // #ifdef UNICODE
		return FALSE;
	return TRUE;
}


static void ConvertWin32FindToFSYS(const WIN32_FIND_DATA *pFD, FSYS_FINDFILE_INFO *pInfo)
{
    WORD wDate;
    WORD wTime;
    struct tm tmTime;

#ifdef UNICODE
    assert(wcslen(pFD->cFileName) < MAX_PATH);
    sprintf(pInfo->pszFName, "%S", pFD->cFileName);
#else // #ifdef UNICODE
	assert(strlen(pFD->cFileName) < MAX_PATH);
	strcpy(pInfo->pszFName, pFD->cFileName);
#endif // #ifdef UNICODE
	if(pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		pInfo->fIsDirectory = TRUE;
	else
		pInfo->fIsDirectory = FALSE;
	pInfo->dwFileSize = pFD->nFileSizeLow;
	assert(pFD->nFileSizeHigh == 0);

    FileTimeToDosDateTime(&pFD->ftCreationTime, &wDate, &wTime);
    memset(&tmTime, 0x00, sizeof(struct tm));
    tmTime.tm_min  = ((wTime>> 5)&0x3F);
    tmTime.tm_hour = ((wTime>>11)&0x1F);
    tmTime.tm_mday = ((wDate    )&0x1F);
    tmTime.tm_mon  = ((wDate>> 5)&0x0F);
    tmTime.tm_mon--;
    tmTime.tm_year = ((wDate>> 9)&0x7F);
    tmTime.tm_year += 80;
    pInfo->tCreationTime = mktime(&tmTime);

}

FS_FINDFILE_HANDLE FSYS_FindDirContents(const char *pszDirectory, FSYS_FINDFILE_INFO *pInfo)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	char pszDirectory2[MAX_PATH+10];

	FSYS_DEBUG(("FSYS_FindDirContents %s\r\n", pszDirectory));

	assert(pszDirectory);
	assert(pInfo);
#ifdef _DEBUG
	memset(pInfo, 0xDD, sizeof(FSYS_FINDFILE_INFO));
#endif

    strcpy(pszDirectory2, pszDirectory);

	if(strchr(pszDirectory,'*') || strchr(pszDirectory,'?')) {
		// do not mess with the path
	} else {
		if(strlen(pszDirectory)>1)
			strcat(pszDirectory2,"\\*.*");
		else
			strcat(pszDirectory2,"*.*");
	}

#ifdef UNICODE
    wchar_t wpszDirectory2[MAX_PATH+1];
    swprintf(wpszDirectory2, L"%S", pszDirectory2);
    hFind = FindFirstFile(wpszDirectory2, &fd);
#else // #ifdef UNICODE
	hFind = FindFirstFile(pszDirectory2, &fd);
#endif // #ifdef UNICODE
	if(hFind == INVALID_HANDLE_VALUE)
		return INVALID_FS_FINDFILE_HANDLE;

#ifdef UNICODE
    while((wcscmp(fd.cFileName, L".")==0) || (wcscmp(fd.cFileName, L"..")==0))
#else // #ifdef UNICODE
	while((strcmp(fd.cFileName,".")==0) || (strcmp(fd.cFileName,"..")==0))
#endif // #ifdef UNICODE
	{
		if(FindNextFile(hFind, &fd) == 0) {
			FindClose(hFind);
			return INVALID_FS_FINDFILE_HANDLE;
		}
	}

	ConvertWin32FindToFSYS(&fd, pInfo);

	return (FS_FINDFILE_HANDLE)hFind;
}


BOOL FSYS_FindNextDirEntry(FS_FINDFILE_HANDLE hFind, FSYS_FINDFILE_INFO *pInfo)
{
	WIN32_FIND_DATA fd;

	FSYS_DEBUG(("FSYS_FindNextDirEntry Handle%08X\r\n", (int)hFind));

	assert(hFind);
	assert(pInfo);
#ifdef _DEBUG
	memset(pInfo, 0xDD, sizeof(FSYS_FINDFILE_INFO));
#endif

	if(FindNextFile(hFind, &fd) == 0) {		
		return FALSE;
	}

	ConvertWin32FindToFSYS(&fd, pInfo);

	return TRUE;
}

void FSYS_FindClose(FS_FINDFILE_HANDLE hFind)
{
	assert(hFind);
	FSYS_DEBUG(("FSYS_FindClose Handle%08X\r\n", (int)hFind));
	FindClose((HANDLE)hFind);
}
