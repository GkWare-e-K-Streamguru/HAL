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
#ifndef __HAL_FILESYSTEM_H
#define __HAL_FILESYSTEM_H


#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
	
/////////////////////////////////////////////
// Generic filesystem API
//! @file HAL_FileSystem.h
//! @brief Platform-independent filesystem APIs


////////////////////////////////////////////////////////////
//! @name File related types and functions
//! 
//@{

typedef void* FILE_HANDLE;			//!< a file handle
#define INVALID_FILE_HANDLE NULL	//!< returned by #FSYS_Open in case of an error

#if _FILE_OFFSET_BITS == 64
typedef QWORD FILESIZE_TYPE;		//!< 64bit file size supported
#else
typedef DWORD FILESIZE_TYPE;		//!< only 32bit file size (~4GB)
#endif

//! FSYS_FILEPOS_MODE contains the possible modes of a seek operation
typedef enum {
	FSYS_FP_START = 1,	//!< the seek position is given relative to the file beginning
	FSYS_FP_CUR   = 2,	//!< the seek position is given relative to the current file position
	FSYS_FP_END   = 3	//!< the seek position is given relative to the file end
} FSYS_FILEPOS_MODE;

//! FSYS_Open creates a new file handle.
//! Handles created using this function must be closed with FSYS_Close.
//! @param pszFName Name of the file
//! @param fWriteAccess TRUE for write-access. If this parameter is TRUE and the file does not exist, it will be created. FALSE for read-only access.
//! @return A new file handle or INVALID_FILE_HANDLE in case of an error
FILE_HANDLE FSYS_Open(const char *pszFName, BOOL fWriteAccess);

//! FSYS_Read reads data from a file
//! @param hFile File handle opened using #FSYS_Open
//! @param pBuf Buffer that file contents should be read into
//! @param dwLen The number of bytes that should be read.
//! @return The number of bytes that have been read, 0 if the end of the file has been reached or -1 in case of an error
int FSYS_Read(FILE_HANDLE hFile, void *pBuf, DWORD dwLen);

//! FSYS_Write writes data into a file
//! @param hFile File handle opened using #FSYS_Open
//! @param pBuf Buffer containing data that should be written
//! @param dwLen The number of bytes to write
//! @return The number of bytes that have been written, -1 in case of an error
int FSYS_Write(FILE_HANDLE hFile, const void *pBuf, DWORD dwLen);

//! FSYS_GetFileSize returns the size of a file in bytes
//! @param hFile File handle opened using #FSYS_Open
FILESIZE_TYPE FSYS_GetFileSize(FILE_HANDLE hFile);

//! FSYS_SetFilePos sets the read/write pointer of a file
//! @param hFile File handle opened using #FSYS_Open
//! @param nPos The relative position to seek to
//! @param eMode Defines how the absolute position is calculated out of the nPos parameter
//! @return the position inside the file after the repositioning or a negative value in case of an error
int FSYS_SetFilePos(FILE_HANDLE hFile, int nPos, FSYS_FILEPOS_MODE eMode);

//! FSYS_Close closes a file handle, freeing all resources related to it.
//! @param hFile File handle opened using #FSYS_Open
void FSYS_Close(FILE_HANDLE hFile);

//! FSYS_DeleteFile deletes a file
//! @return TRUE if the file was deleted successfully, FALSE in case of an error
BOOL FSYS_DeleteFile(const char *pszFName);

//@}


////////////////////////////////////////////////////////////
//! @name Directory and directory enumeration related types and functions
//! 
//@{

//! FSYS_CreateDirectory creates a directory
//! @param pszDirectory Name of the new directory
//! @return TRUE if the directory was created successfully
BOOL FSYS_CreateDirectory(const char *pszDirectory);

//! FSYS_DeleteDirectory deletes a directory
//! @param pszDirectory Name of the new directory
//! @return TRUE if the directory was deleted successfully
BOOL FSYS_DeleteDirectory(const char *pszDirectory);


//
// Directory enumeration
//
typedef void* FS_FINDFILE_HANDLE;			//!< a file enumeration handle
#define INVALID_FS_FINDFILE_HANDLE NULL		//!< returned by FSYS_FindDirContents in case of an error

//! FSYS_FINDFILE_INFO contains information that can be obtained about a file
//! as part of a directory content enumeration
typedef struct {
	char pszFName[MAX_PATH];	//!< the filename (only the filename, no path/directory)
	FILESIZE_TYPE dwFileSize;	//!< the file size in bytes (zero for a directory) (**TODO** large file support)
	BOOL fIsDirectory;			//!< TRUE if this describes a directory entry instead of a file
    time_t tCreationTime;       //!< The file creation time given as time_t type as returned by the ANSI C-runtime function time()
} FSYS_FINDFILE_INFO;

//! FSYS_FindDirContents starts a directory content enumeration.
//! The behavior if directory contents change between the calls to #FSYS_FindDirContents and #FSYS_FindClose is undefined.
//! Enumeration handles returned by this function must be closed by a call to #FSYS_FindClose.
//! This function does not return special files like "." and "..".
//! @param pszDirectory Name of the directory whose contents should be enumerated
//! @param pInfo Output parameter for the first entry
//! @return A new file enumeration handle if the pInfo parameter has been filled with the first enumeration result or 
//!         INVALID_FS_FINDFILE_HANDLE if the directory is empty or in case of an error.
FS_FINDFILE_HANDLE FSYS_FindDirContents(const char *pszDirectory, FSYS_FINDFILE_INFO *pInfo);

//! FSYS_FindNextDirEntry reads the next file or directory entry.
//! @param hFind A handle returned by FSYS_FindDirContents.
//! @param pInfo Output parameter for the next file entry
//! @return TRUE if the pInfo parameter has been filled with results, FALSE if the enumeration is complete or if an error has occured.
BOOL FSYS_FindNextDirEntry(FS_FINDFILE_HANDLE hFind, FSYS_FINDFILE_INFO *pInfo);

//! FSYS_FindClose completes a directory content enumeration and frees all resources
//! that have been allocated for it.
void FSYS_FindClose(FS_FINDFILE_HANDLE hFind);

//@}


// **************************************************
//   Platform-Independent HAL_FileSystem utilities

#ifndef FSYSUTIL_LOADFILE_LIMIT
#define FSYSUTIL_LOADFILE_LIMIT (32*1024*1024)	//!< The default size limit for #FSYSUtil_LoadFile calls.
#endif

//! FSYSUtil_LoadFile attempts to open a file, allocate enough memory, read it entirely and return the buffer.
//! Files are limited to FSYSUTIL_LOADFILE_LIMIT bytes (which defaults to 32MB) for safety reasons.
//! Returned buffers need to be released through a call to #FSYSUtil_FreeBuffer.
//! @param pszFile Name of the file to load.
//! @param pdwOutLen (mandatory) Out-Parameter for the file size
//! @return A pointer to a buffer holding the file contents or NULL if any kind of problem occurred.
BYTE *FSYSUtil_LoadFile(const char *pszFile, FILESIZE_TYPE *pdwOutLen);

//! FSYSUtil_FreeBuffer releases all resources attached to a buffer returned by #FSYSUtil_LoadFile.
//! It is safe to call this function with NULL as parameter (Debug builds will assert but not crash).
//! @param pBuf Pointer to the buffer that should be freed.
void  FSYSUtil_FreeBuffer(BYTE *pBuf);

//! FSYSUtil_WriteFile writes a block of memory into a file and returns TRUE
//! if the entire operation was successful.
BOOL FSYSUtil_WriteFile(const char *pszFile, const BYTE *pBuffer, DWORD dwLen);

typedef BOOL (LINEPROCESSINGCALLBACK)(void *pCookie, char *pszLine);

BOOL FSYSUtil_ProcessFileByLine(const char *pszFile, void *pCookie, LINEPROCESSINGCALLBACK CB);

typedef BOOL (FINDFILEPROCESSINGCALLBACK)(void *pCookie, FSYS_FINDFILE_INFO *pFind);

BOOL FSYSUtil_ProcessDirectoryContents(const char *pszDirectory, void *pCookie, FINDFILEPROCESSINGCALLBACK CB);

#ifdef __cplusplus
}
#endif

#endif // __HAL_FILESYSTEM_H
