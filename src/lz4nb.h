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

#ifndef LZ4NB_H
#define LZ4NB_H

#include <windows.h>

#include "../include/lz4/lz4frame.h"
#include "../include/lz4/lz4frame_static.h"

// 구조체 선언

typedef struct LZ4_NB_Core_s LZ4_NB_Core_t;
typedef struct LZ4_NB_Context_s LZ4_NB_Context_t;

struct LZ4_NB_Core_s {
    HANDLE hInput;            // 입력 핸들
    HANDLE hOutput;           // 출력 핸들
    LZ4F_cctx* cctxPtr;       // LZ4F 압축 컨텍스트 포인터
    LPVOID srcBuf;            // 원본 데이터 버퍼
    size_t srcBufMaxSize;     // 원본 데이터 버퍼의 최대 크기
    LPVOID dstBuf;            // 압축된 데이터 버퍼
    size_t dstBufMaxSize;     // 압축된 데이터 버퍼의 최대 크기
    DWORD dwTotalChunks;      // 총 청크 수
    BOOL bWait;               // File I/O 작업 시, 대기 여부 (Blocking: TRUE, Non-Blocking: FALSE)
};

struct LZ4_NB_Context_s {
    LZ4_NB_Core_t* lz4NB;            // Non-Blocking LZ4 Core 구조체 포인터
    OVERLAPPED readOverlap;          // Non-Blocking 읽기 작업을 위한 OVERLAPPED 구조체
    OVERLAPPED writeOverlap;         // Non-Blocking 쓰기 작업을 위한 OVERLAPPED 구조체
};

// 함수 선언
void LZ4F_freeNB(LZ4_NB_Core_t* lz4NB);
BOOL LZ4F_createNB(
    LZ4_NB_Core_t** lz4NB,
    HANDLE hInput, HANDLE hOutput,
    size_t srcSize, DWORD dwTotalChunks, BOOL bWait
);
BOOL LZ4F_NB_Begin(LZ4_NB_Context_t* lz4nbCtx);
BOOL LZ4F_NB_Process(LZ4_NB_Context_t* lz4nbCtx);
BOOL LZ4F_NB_Finalize(LZ4_NB_Context_t* lz4nbCtx);

BOOL LZ4F_NB_Compress(LZ4_NB_Core_t* lz4NB);

BOOL compress_lz4(const TCHAR* inputFilePath, const TCHAR* outputFilePath);

#endif // LZ4NB_H
