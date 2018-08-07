#pragma once

#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include "VPStructs.h"

#define BUFFERSIZE 65536
#define DIRENTRY_ARR_SIZE 1024
#define REALLOC_ADDITION 256
#define MAX_RECURSION_DEPTH 8
//success/error defines
#define SUCCESS 0
#define USER_ERROR -1
#define CREATEFILE_ERROR 1
#define READFILE_ERROR 2
#define WRITEFILE_ERROR 3
#define TEMP_FILE_ERROR 4
#define MOVE_FILE_ERROR 5
#define RECURSION_DEPTH_EXCEEDED 6

int writeVP(HANDLE writeHandle, char* *readBasePath, DWORD *amtBuffered, int depth);
BOOL writeHeader(HANDLE file, int offset, int entries);
void initializeHeader(header *head, int offset, int entries);
BOOL writeDirentries(HANDLE file, int offset, int numEntries);
void initializeDirentry(direntry *de, int size, LPCSTR name, FILETIME *timestamp);
int convertToUnixTime(FILETIME *timestamp);

direntry *direntryArr;
size_t direntryArrSlots;
//FIXME verify type
BYTE *buf;
DWORD counter, offset;
WIN32_FIND_DATAA fileInfo;