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
#include "HAL_FileSystem.h"



BYTE *FSYSUtil_LoadFile(const char *pszFile, FILESIZE_TYPE *pdwOutLen)
{
	FILE_HANDLE hFile;
	BYTE *pBuf;

	assert(pszFile);
	assert(pdwOutLen);
	if(!pszFile || !pdwOutLen)
		return NULL;

	hFile = FSYS_Open(pszFile, FSYS_READONLY);
	if(hFile == INVALID_FILE_HANDLE) {
		// UART_printf("File not found\r\n");
		return NULL;
	}
	*pdwOutLen = FSYS_GetFileSize(hFile);
	assert((*pdwOutLen) <= FSYSUTIL_LOADFILE_LIMIT);
	if(*pdwOutLen > FSYSUTIL_LOADFILE_LIMIT) {
		// UART_printf("File %s over size limit (%d/%d)\r\n",pszFile, (int)*pdwOutLen,FSYSUTIL_LOADFILE_LIMIT);
		return NULL;
	}

	pBuf = (BYTE*)malloc((size_t)*pdwOutLen+1); // +1 for terminating zero
	assert(pBuf);
	if(!pBuf) {
		FSYS_Close(hFile);
		return NULL;
	}
	if(FSYS_Read(hFile, pBuf, (DWORD)*pdwOutLen) != (int)(*pdwOutLen)) {
		// UART_printf("File %s read error\r\n", pszFile);
		free(pBuf);
		return NULL;
	}
    pBuf[*pdwOutLen] = 0x00; // terminating zero
	FSYS_Close(hFile);
	return pBuf;
}

void  FSYSUtil_FreeBuffer(BYTE *pBuf)
{
	assert(pBuf);
	if(!pBuf)
		return;
	free(pBuf);
}


BOOL FSYSUtil_ProcessFileByLine(const char *pszFile, void *pCookie, LINEPROCESSINGCALLBACK pProcessingCB)
{
	FILESIZE_TYPE dwLen;
	BYTE *pFileBuf = FSYSUtil_LoadFile(pszFile, &dwLen);
	//assert(pFileBuf); // it is legitimate to call this when the file does not exist, fires annoyingly in EPGTX project
	if(!pFileBuf)
		return FALSE;
	
	// convert all \r and \n to \0
	for(DWORD i=0;i<dwLen;i++) {
		if((pFileBuf[i]=='\r') || (pFileBuf[i]=='\n'))
			pFileBuf[i] = '\0';		
	}
	
	// loop over the file content
	for(char *c = (char *)pFileBuf; c<(char *)(pFileBuf+dwLen); c++)
	{
		const size_t nLineLen = strlen(c);
		if(strlen(c)) {
			if(!pProcessingCB(pCookie, c)) {
				FSYSUtil_FreeBuffer(pFileBuf);
				return FALSE;
			}
		}		
		
		c += nLineLen;
	}
	
	FSYSUtil_FreeBuffer(pFileBuf);
	return TRUE;
}

BOOL FSYSUtil_WriteFile(const char *pszFile, const BYTE *pBuffer, DWORD dwLen)
{
	FILE_HANDLE hFile = FSYS_Open(pszFile, FSYS_READWRITE);
	if(hFile == INVALID_FILE_HANDLE)
		return FALSE;
	DWORD dwWritten = FSYS_Write(hFile, pBuffer, dwLen);
	if(dwWritten != dwLen) {
		FSYS_Close(hFile);
		FSYS_DeleteFile(pszFile);
		return FALSE;
	}
	FSYS_Close(hFile);
	return TRUE;
}

BOOL FSYSUtil_ProcessDirectoryContents(const char *pszDirectory, void *pCookie, FINDFILEPROCESSINGCALLBACK CB)
{
	FSYS_FINDFILE_INFO Info;
	FS_FINDFILE_HANDLE hFind = FSYS_FindDirContents(pszDirectory, &Info);
	if(hFind == INVALID_FS_FINDFILE_HANDLE)
		return FALSE;

	while(hFind != INVALID_FS_FINDFILE_HANDLE)
	{
		if(    !CB(pCookie, &Info)
			|| !FSYS_FindNextDirEntry(hFind, &Info))
		{
			FSYS_FindClose(hFind);
			hFind = INVALID_FS_FINDFILE_HANDLE;
		}
	}

	return TRUE;
}
