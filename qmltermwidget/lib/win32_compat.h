/*
 * Windows compatibility layer for POSIX functions used by the terminal.
 * Provides mmap/munmap/getpagesize emulation for BlockArray and History.
 *
 * This file does NOT define POSIX I/O macros (lseek, read, write, close, etc.)
 * Those are handled locally in BlockArray.cpp where needed.
 *
 * Copyright (C) 2024 pushingpandas
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// mmap/munmap emulation
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define MAP_PRIVATE   0x02
#define MAP_ANON      0x20
#define MAP_FAILED    ((void *)-1)

inline void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (flags & MAP_ANON) {
        // Anonymous mapping - use VirtualAlloc
        Q_UNUSED(fd)
        Q_UNUSED(offset)
        DWORD protect = PAGE_READWRITE;
        if (!(prot & PROT_WRITE))
            protect = PAGE_READONLY;
        void *ptr = VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, protect);
        return ptr ? ptr : MAP_FAILED;
    }

    Q_UNUSED(addr)
    Q_UNUSED(flags)

    DWORD flProtect = PAGE_READONLY;
    DWORD dwDesiredAccess = FILE_MAP_READ;

    if (prot & PROT_WRITE) {
        flProtect = PAGE_READWRITE;
        dwDesiredAccess = FILE_MAP_WRITE;
    }

    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE)
        return MAP_FAILED;

    HANDLE hMapping = CreateFileMappingA(hFile, NULL, flProtect, 0, 0, NULL);
    if (!hMapping)
        return MAP_FAILED;

    DWORD offsetHigh = (DWORD)(((unsigned long long)offset >> 32) & 0xFFFFFFFF);
    DWORD offsetLow = (DWORD)((unsigned long long)offset & 0xFFFFFFFF);

    void *ptr = MapViewOfFile(hMapping, dwDesiredAccess, offsetHigh, offsetLow, length);
    CloseHandle(hMapping);

    if (!ptr)
        return MAP_FAILED;

    return ptr;
}

inline int munmap(void *addr, size_t length)
{
    Q_UNUSED(length)
    // Try UnmapViewOfFile first (for file-backed mappings)
    if (UnmapViewOfFile(addr))
        return 0;
    // Fall back to VirtualFree (for anonymous mappings)
    if (VirtualFree(addr, 0, MEM_RELEASE))
        return 0;
    return -1;
}

inline int getpagesize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<int>(si.dwPageSize);
}

#endif // Q_OS_WIN

#endif // WIN32_COMPAT_H
