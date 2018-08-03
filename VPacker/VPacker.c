#include "VPacker.h"

//TODO write to temp file and then copy to output path

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <[Path\\To\\]Output.vp> <[Path\\To\\]InputFolder>", argv[0]);
		return USER_ERROR;
	}

	HANDLE writeHandle;
	LPCSTR writeFilePath = (LPCSTR)_malloca(strnlen(argv[1], MAX_PATH) + 1);
	writeFilePath = argv[1];
	char *readBasePath = (char*)malloc(MAX_PATH + 1);
	strcpy_s(readBasePath, MAX_PATH, argv[2]);

	LPCSTR folderName = strrchr(readBasePath, '\\') + 1;
	if (_stricmp(folderName, "data")) {
		printf("The root folder must be named \"data\"");
		return USER_ERROR;
	}

	LPCSTR fileExtension = strrchr(writeFilePath, '.') + 1;
	if (_stricmp(fileExtension, "vp") && _stricmp(fileExtension, "VP")) {
		printf("The output file must be a .vp file");
		return USER_ERROR;
	}

	counter = 0;
	offset = sizeof(header);

	int rv;

	direntryArrSlots = DIRENTRY_ARR_SIZE;
	direntryArr = (direntry *)malloc(DIRENTRY_ARR_SIZE * sizeof(direntry));

	writeHandle = CreateFileA(writeFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (writeHandle == INVALID_HANDLE_VALUE) {
		rv = 1;
	}
	else {
		//make room for the header
		LARGE_INTEGER moveDist;
		moveDist.QuadPart = sizeof(header);
		SetFilePointerEx(writeHandle, moveDist, NULL, FILE_BEGIN);

		//create the direntry for the given folder
		FILETIME zero;
		zero.dwHighDateTime = 0;
		zero.dwLowDateTime = 0;
		initializeDirentry(&(direntryArr[counter++]), 0, folderName, &zero);
		rv = writeVP(writeHandle, &readBasePath, 0);

		writeHeader(writeHandle, offset, counter);

		int tempOffset = sizeof(header);
		//fill in the offsets now that we can calculate them
		for (int i = 0; i < counter; i++) {
			direntryArr[i].offset = tempOffset;
			tempOffset += direntryArr[i].size;
		}
		writeDirentries(writeHandle, offset, counter);

		FlushFileBuffers(writeHandle);
	}

	CloseHandle(writeHandle);
	free(direntryArr);
	direntryArr = NULL;
	free(readBasePath);
	readBasePath = NULL;
	if (!rv) {
		printf("Success");
	}
	else if (rv == CREATEFILE_ERROR) {
		printf("Error creating a necessary file");
	}
	else if (rv == READFILE_ERROR) {
		printf("Error reading a file");
	}
	else if (rv == WRITEFILE_ERROR) {
		printf("Error writing the vp file");
	}
	else if (rv == RECURSION_DEPTH_EXCEEDED) {
		printf("Recursion depth exceeded; perhaps you have too many nested folders");
	}
	return rv;
}

int writeVP(HANDLE writeHandle, char* *readBasePath, int depth) {
	if (depth > MAX_RECURSION_DEPTH) {
		return RECURSION_DEPTH_EXCEEDED;
	}

	int rv = 0;

	char readFilePath[MAX_PATH];
	DWORD numRead = 0;
	DWORD numWritten = 0;
	HANDLE readHandle, fileSearchHandle;

	strcpy_s(readFilePath, MAX_PATH, *readBasePath);
	strcat_s(readFilePath, MAX_PATH, "\\*");
	fileSearchHandle = FindFirstFileA(readFilePath, &fileInfo);

	FILETIME zero;
	zero.dwHighDateTime = 0;
	zero.dwLowDateTime = 0;

	do {
		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//exclude current and parent directories
			//current directory because it's directry has already been written
			//and parent for obvious reasons
			if (_stricmp(fileInfo.cFileName, ".") && _stricmp(fileInfo.cFileName, "..")) {
				initializeDirentry(&(direntryArr[counter++]), 0, fileInfo.cFileName, &zero);
				strcat_s(*readBasePath, MAX_PATH, "\\");
				strcat_s(*readBasePath, MAX_PATH, fileInfo.cFileName);
				writeVP(writeHandle, readBasePath, depth + 1);

				char *c = strrchr(*readBasePath, '\\');
				*c = 0;
			}
			continue;
		}
		strcpy_s(readFilePath, MAX_PATH, *readBasePath);
		strcat_s(readFilePath, MAX_PATH, "\\");
		strcat_s(readFilePath, MAX_PATH, fileInfo.cFileName);
		readHandle = CreateFileA(readFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (readHandle == INVALID_HANDLE_VALUE) {
			rv = CREATEFILE_ERROR;
			goto cleanup;
		}
		strcpy_s(readFilePath, MAX_PATH, *readBasePath);
		strcat_s(readFilePath, MAX_PATH, "\\*");

		//Potential for overflow but the VP spec specifies an int
		int fileSize = (fileInfo.nFileSizeHigh * (MAXDWORD + 1)) + fileInfo.nFileSizeLow;

		initializeDirentry(&(direntryArr[counter++]), fileSize, fileInfo.cFileName, &(fileInfo.ftLastWriteTime));

		offset += fileSize;

		do {
			if (!ReadFile(readHandle, &buf, BUFFERSIZE, &numRead, NULL)) {
				rv = READFILE_ERROR;
				goto cleanup;
			}
			if (!WriteFile(writeHandle, buf, numRead, &numWritten, NULL)) {
				rv = WRITEFILE_ERROR;
				goto cleanup;
			}
		} while (numRead);

		if (counter > direntryArrSlots) {
			direntryArrSlots += REALLOC_ADDITION;
			realloc(direntryArr, direntryArrSlots);
		}
		CloseHandle(readHandle);
	} while (FindNextFileA(fileSearchHandle, &fileInfo));

	initializeDirentry(&(direntryArr[counter]), 0, "..", &zero);
	counter++;

cleanup:
	FindClose(fileSearchHandle);
	return rv;
}

BOOL writeHeader(HANDLE file, int offset, int entries) {
	LARGE_INTEGER moveDist;
	moveDist.QuadPart = 0;
	if (!SetFilePointerEx(file, moveDist, NULL, FILE_BEGIN)) {
		return FALSE;
	}

	header head;
	DWORD numWritten;
	initializeHeader(&head, offset, entries);
	if (WriteFile(file, (LPVOID)(&head), sizeof(head), &numWritten, NULL)) {
		return FALSE;
	}

	return TRUE;
}

void initializeHeader(header *head, int offset, int entries) {
	head->signature[0] = 'V';
	head->signature[1] = 'P';
	head->signature[2] = 'V';
	head->signature[3] = 'P';
	head->version = 2;
	head->diroffset = offset;
	head->direntries = entries;
}

BOOL writeDirentries(HANDLE file, int offset, int numEntries) {
	LARGE_INTEGER moveDist;
	DWORD numWritten;

	//reset the file pointer for writing the direntries
	moveDist.QuadPart = offset;
	if (!SetFilePointerEx(file, moveDist, NULL, FILE_BEGIN)) {
		return FALSE;
	}

	if (WriteFile(file, (LPVOID)(direntryArr), numEntries * sizeof(direntry), &numWritten, NULL)) {
		return FALSE;
	}
	return TRUE;
}

//This function isn't given the offset because when it's called, we don't know the offset yet
void initializeDirentry(direntry *de, int size, LPCSTR name, FILETIME *timestamp) {
	de->size = size;
	strcpy_s(de->filename, 32, name);
	de->timestamp = convertToUnixTime(timestamp);
}

//https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
int convertToUnixTime(FILETIME *timestamp) {
	if (timestamp->dwHighDateTime == 0 && timestamp->dwLowDateTime == 0) {
		return 0;
	}

	LONGLONG unixZeroVal = 116444736000000000;
	LARGE_INTEGER unixZero;
	ULARGE_INTEGER givenTime;
	unixZero.LowPart = (DWORD)unixZeroVal;
	unixZero.HighPart = unixZeroVal >> 32;
	givenTime.LowPart = timestamp->dwLowDateTime;
	givenTime.HighPart = timestamp->dwHighDateTime;
	givenTime.QuadPart -= unixZero.QuadPart;
	givenTime.QuadPart /= 10000000;
	return (int)givenTime.QuadPart;
}