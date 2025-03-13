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

#ifndef ZSTD_NB_H
#define ZSTD_NB_H

#include "../include/zstd/zstd.h"      // presumes zstd library is installed

#include <windows.h>

// 구조체 선언

typedef struct resources_s resources_t;

struct resources_s {
    LPVOID srcBuf;
    size_t srcBufMaxSize;
    LPVOID dstBuf;
    size_t dstBufMaxSize;
    ZSTD_CCtx* cctxPtr;
    BOOL bWait;
};

// 함수 선언

BOOL create_resources(resources_t** ress);
void free_resources(resources_t* ress);
BOOL ZSTD_NB_Process(resources_t* ress, HANDLE hInput, HANDLE hOutput);
BOOL compress_zstd(const TCHAR* fname, const TCHAR* outName);

#endif // ZSTD_NB_H
