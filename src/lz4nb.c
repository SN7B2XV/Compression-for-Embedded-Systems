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

#include "lz4nb.h"
#include "asyncio_win.h"
#include "utility.h"

#define CHUNK_SIZE (16 * 1024) // 읽기/쓰기 블록 크기 (16 KB)

// 압축 옵션 설정
static const LZ4F_preferences_t kPrefs = {
    { 
        LZ4F_max64KB,
        LZ4F_blockLinked,
        LZ4F_noContentChecksum,
        LZ4F_frame,
        0, // Unknown size of uncompressed content
        0, // No dictionary ID
        LZ4F_noBlockChecksum
    }, // Frame info
    0, // Compression level. Default 는 0
    0, // Auto flush
    0, // Favor decompression speed
    { 0, 0, 0 },  // reserved. 0 으로 설정해야함
};

/**
* @brief Non-Blocking LZ4 압축 작업의 자원을 정리합니다.
*
* 이 함수는 Non-Blocking 압축 작업의 관련된 자원을 해제합니다.
* 압축 작업이 완료된 후 호출하여 자원을 정리합니다.
*
* @param lz4NB LZ4 Non-Blocking 작업 구조체 포인터
* @return 종료 성공 시 TRUE, 실패 시 FALSE
*/
void LZ4F_freeNB(LZ4_NB_Core_t* lz4NB)
{
    if (lz4NB == NULL) {
        return;
    }

    // 파일 작업 완료 후 자원 정리
    LZ4F_freeCompressionContext(lz4NB->cctxPtr);
    free(lz4NB->srcBuf);
    free(lz4NB->dstBuf);
    free(lz4NB);
}

/**
* @brief Non-Blocking LZ4 압축 작업을 위한 구조체를 초기화하고 파일 핸들을 설정합니다.
*
* 이 함수는 입력 파일과 출력 파일 핸들을 받아 Non-Blocking LZ4 압축 작업을 준비합니다.
* 작업을 시작하기 전에 필요한 모든 자원과 설정을 할당하고 초기화합니다.
*
* @param lz4NB LZ4 Non-Blocking 작업 구조체 이중 포인터
* @param hInput 입력 파일 핸들
* @param hOutput 출력 파일 핸들
* @param srcSize 입력 데이터 크기
* @param dwTotalChunks 총 청크 수
* @param bWait File I/O 작업 시, 대기 여부 (Blocking: TRUE, Non-Blocking: FALSE)
* @return 성공 시 TRUE, 실패 시 FALSE
*/
BOOL LZ4F_createNB(
    LZ4_NB_Core_t** lz4NB,
    HANDLE hInput, HANDLE hOutput,
    size_t srcSize, DWORD dwTotalChunks, BOOL bWait
) {
    // 자원 할당
    *lz4NB = (LZ4_NB_Core_t*)calloc(1, sizeof(LZ4_NB_Core_t));

    (*lz4NB)->hInput = hInput;
    (*lz4NB)->hOutput = hOutput;
    (*lz4NB)->dwTotalChunks = dwTotalChunks;
    (*lz4NB)->bWait = bWait;
    
    size_t const cctxCreation = LZ4F_createCompressionContext(&((*lz4NB)->cctxPtr), LZ4F_VERSION);

    (*lz4NB)->srcBufMaxSize = srcSize;
    (*lz4NB)->srcBuf = malloc((*lz4NB)->srcBufMaxSize); // 읽을 데이터 버퍼
    (*lz4NB)->dstBufMaxSize = LZ4F_compressBound(srcSize, &kPrefs); // 충분히 큰 크기로 설정 (<= srcSize)
    (*lz4NB)->dstBuf = malloc((*lz4NB)->dstBufMaxSize); // 압축하여 저장할 데이터 버퍼 
    
    if (!LZ4F_isError(cctxCreation) &&
        (*lz4NB)->srcBuf && (*lz4NB)->dstBuf &&
        ((*lz4NB)->dstBufMaxSize >= LZ4F_HEADER_SIZE_MAX)) { 
        return TRUE;
    }

    log_message("Failed to start compression (parameter)...");

    LZ4F_freeNB(*lz4NB);
    *lz4NB = NULL;

    return FALSE;
}

/**
 * @brief Frame header를 Non-Blocking 방식으로 씁니다.
 *
 * @param lz4NB LZ4 Non-Blocking 작업 구조체
 * @return Non-Blocking 작업 성공 여부
 */
BOOL LZ4F_NB_Begin(LZ4_NB_Context_t* lz4nbCtx) {
    DWORD dwBytesWritten;
    LZ4_NB_Core_t* lz4NB = lz4nbCtx->lz4NB;

    size_t const headerSize = LZ4F_compressBegin(lz4NB->cctxPtr, lz4NB->dstBuf, lz4NB->dstBufMaxSize, &kPrefs);
    if (LZ4F_isError(headerSize)) {
        log_message("Failed to start compression (header)...");
        return FALSE;
    }

    BOOL bResult = async_write(
        lz4NB->hOutput, lz4NB->dstBuf, (DWORD)headerSize,
        &dwBytesWritten, &(lz4nbCtx->writeOverlap), TRUE
    );
    if (bResult == FALSE) {
        log_message("Writing-->failed...");
        return FALSE;
    }

    lz4nbCtx->writeOverlap.Offset += dwBytesWritten; // 쓴 만큼 오프셋 갱신
    return TRUE;
}

/**
 * @brief 파일을 Non-Blocking 방식으로 읽고, 압축하여 파일에 씁니다.
 *
 * @param lz4NB LZ4 Non-Blocking 작업 구조체
 * @return Non-Blocking 작업 성공 여부
 */
BOOL LZ4F_NB_Process(LZ4_NB_Context_t* lz4nbCtx) {
    BOOL bResult;
    DWORD dwBytesRead, dwBytesWritten;
    size_t compressedSize;
    LZ4_NB_Core_t* lz4NB = lz4nbCtx->lz4NB;

    for (DWORD chunk = 0; chunk < lz4NB->dwTotalChunks; chunk++) {
        
        // 1. 원본 파일 읽기
        bResult = async_read(
            lz4NB->hInput, lz4NB->srcBuf, lz4NB->srcBufMaxSize,
            &dwBytesRead, &(lz4nbCtx->readOverlap), TRUE
        ); // 압축을 진행하기 위해서는 대상 정보가 필요하기에, 읽기는 항상 동기식으로 진행
        if (bResult == FALSE) {
            break;  // 오류 발생 시 종료
        }
        if (dwBytesRead == 0) {
            break;  // EOF 발생 시 종료
        }

        lz4nbCtx->readOverlap.Offset += dwBytesRead; // 읽은 만큼 오프셋 갱신

        // 2. 읽은 내용 압축하기
        compressedSize = LZ4F_compressUpdate(
            lz4NB->cctxPtr, lz4NB->dstBuf, lz4NB->dstBufMaxSize,
            lz4NB->srcBuf, dwBytesRead, NULL
        );
        if (LZ4F_isError(compressedSize)) {
            log_message("Compression failed: error...");
            return FALSE;
        }

        // 3. 압축한 내용 쓰기
        if (chunk > 0) {
            if (!lz4NB->bWait) {
                bResult = GetOverlappedResult(
                    lz4NB->hOutput, &(lz4nbCtx->writeOverlap),
                    &dwBytesWritten, TRUE
                );  // 비동기 작업 결과 확인
            }

            lz4nbCtx->writeOverlap.Offset += dwBytesWritten; // 쓴 만큼 오프셋 갱신
        }

        bResult = async_write(
            lz4NB->hOutput, lz4NB->dstBuf, (DWORD)compressedSize,
            &dwBytesWritten, &(lz4nbCtx->writeOverlap), lz4NB->bWait
        );

        if (bResult == FALSE) {
            break;  // 오류 발생 시 종료
        }

        // Kernel Resources 포화 방지
        // if ((chunk % 16) == 15) {
        //     Sleep(1);
        // }
    }

    return TRUE;
}

/**
 * @brief 압축 종료 및 내부 버퍼를 Non-Blocking 방식으로 플러시합니다.
 *
 * @param lz4NB LZ4 Non-Blocking 작업 구조체
 * @return Non-Blocking 작업 성공 여부
 */
BOOL LZ4F_NB_Finalize(LZ4_NB_Context_t* lz4nbCtx) {
    DWORD dwBytesWritten;
    LZ4_NB_Core_t* lz4NB = lz4nbCtx->lz4NB;
    size_t const compressedSize = LZ4F_compressEnd(lz4NB->cctxPtr, lz4NB->dstBuf, lz4NB->dstBufMaxSize, NULL);
    if (LZ4F_isError(compressedSize)) {
        log_message("Failed to end compression: error...");
        return FALSE;
    }

    BOOL bResult = async_write(
        lz4NB->hOutput, lz4NB->dstBuf, compressedSize,
        &dwBytesWritten, &(lz4nbCtx->writeOverlap), TRUE
    );
    if (bResult == FALSE) {
        log_message("Writing-->failed...");
        return FALSE;
    }

    lz4nbCtx->writeOverlap.Offset += dwBytesWritten; // 쓴 만큼 오프셋 갱신
    return TRUE;
}

/**
 * @brief Non-Blocking 방식으로 읽기와 쓰기 작업을 수행하고, 데이터를 LZ4로 압축하여 파일에 씁니다.
 *
 * @param lz4NB LZ4 Non-Blocking 작업 구조체
 * @return 압축 성공 여부
 */
BOOL LZ4F_NB_Compress(LZ4_NB_Core_t *lz4NB) {
    LZ4_NB_Context_t lz4nbCtx = {
        lz4NB,
        { 0, }, // Non-Blocking 읽기 작업을 위한 OVERLAPPED 구조체
        { 0, }, // Non-Blocking 쓰기 작업을 위한 OVERLAPPED 구조체
    };

    // Frame header 쓰기
    if (!LZ4F_NB_Begin(&lz4nbCtx)) {
        return FALSE;
    }

    // 압축 대상 파일 읽기 및 압축본 쓰기 반복 작업 진행
    if (!LZ4F_NB_Process(&lz4nbCtx)) {
        return FALSE;
    }

    // 압축 Frame 마무리 및 남은 데이터 처리
    if (!LZ4F_NB_Finalize(&lz4nbCtx)) {
        return FALSE;
    }

    // log_message("Compression succeed.");
    return TRUE;
}

/**
* @brief Non-Blocking 방식으로 읽기와 쓰기 작업을 수행하고, 데이터를 LZ4로 압축하여 파일에 씁니다.
*
* 이 함수는 파일을 Non-Blocking 방식으로 읽고, 읽은 데이터를 LZ4 프레임으로 압축 후 다른 파일로 Non-Blocking 방식으로 씁니다.
*
* @param inputFilePath 읽을 파일 경로
* @param outputFilePath 쓸 파일 경로
* @return 압축 성공 여부
*/
BOOL compress_lz4(const TCHAR* inputFilePath, const TCHAR* outputFilePath) {
    BOOL bResult = FALSE;
    
    HANDLE hInput = init_file_read(inputFilePath);  // 읽을 파일
    HANDLE hOutput = init_file_write(outputFilePath);  // 쓸 파일

    // 파일 열기 오류 처리
    if (hInput == INVALID_HANDLE_VALUE) {
        return bResult;
    }

    if (hOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        return bResult;
    }

    DWORD dwFileSize = get_file_size(hInput);  // 파일 크기 얻기
    DWORD dwTotalChunks = dwFileSize / CHUNK_SIZE;
    if (dwFileSize % CHUNK_SIZE != 0) {
        ++dwTotalChunks;
    }

    LZ4_NB_Core_t* lz4NB;
    if (LZ4F_createNB(&lz4NB, hInput, hOutput, CHUNK_SIZE, dwTotalChunks, FALSE)) {
        bResult = LZ4F_NB_Compress(lz4NB);
    } else {
        log_message("error : LZ4 resource allocation failed.");
    }

    // log_message("Compression processing completed.");
    
    // 파일 작업 완료 후 리소스 정리
    CloseHandle(hInput);
    CloseHandle(hOutput);
    LZ4F_freeNB(lz4NB);

    return bResult;
}
