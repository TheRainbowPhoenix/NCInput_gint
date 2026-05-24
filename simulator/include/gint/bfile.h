#ifndef GINT_BFILE_H
#define GINT_BFILE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mock BFile constants
#define BFILE_READ   1
#define BFILE_WRITE  2
#define BFILE_APPEND 3

typedef struct {
    uint16_t id;
    uint32_t size;
    char filename[256];
    uint8_t is_dir;
} bfile_info_t;

// Standard ClassPad file paths look like \\fls0\filename.txt
// In the simulator, we map these to the current working directory or a 'vfs' folder.

int BFile_Open(const uint16_t *filename, int mode);
int BFile_Close(int handle);
int BFile_Read(int handle, void *buffer, int size, int pos);
int BFile_Write(int handle, const void *buffer, int size);
int BFile_Size(int handle);

// Directory enumeration (simplified mock)
int BFile_FindFirst(const uint16_t *pattern, int *handle, bfile_info_t *info);
int BFile_FindNext(int handle, bfile_info_t *info);
int BFile_FindClose(int handle);

#ifdef __cplusplus
}
#endif

#endif
