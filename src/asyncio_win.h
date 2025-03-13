/*
 * Copyright 2025, SN7B2XV

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ASYNCIO_WIN_H
#define ASYNCIO_WIN_H

#include <windows.h>
#include <stdio.h>

// 함수 선언

HANDLE init_file_read(const TCHAR* filePath);
HANDLE init_file_write(const TCHAR* filePath);

BOOL async_read(
    HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead,
    LPDWORD lpBytesRead, LPOVERLAPPED lpOverlap, BOOL bWait
);
BOOL async_write(
    HANDLE hFile, LPCVOID lpBuffer, DWORD dwBytesToWrite,
    LPDWORD lpBytesWritten, LPOVERLAPPED lpOverlap, BOOL bWait
);

#endif // ASYNCIO_WIN_H
