#pragma once

typedef struct {
	char signature[4];
	int version;
	int diroffset;
	int direntries;
} header;

typedef struct {
	int offset;
	int size;
	char filename[32];
	int timestamp;
} direntry;