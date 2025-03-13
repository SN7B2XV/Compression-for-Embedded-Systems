/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/*
 * Modified by SN7B2XV.
 * Portions of the code related to file I/O have been changed.
 */

#include <stdio.h>     // printf
#include <stdlib.h>    // free
#include <string.h>    // memset, strcat, strlen

#include "../include/zstd/zstd.h"      // presumes zstd library is installed

#include "zstd_nb.h"
#include "asyncio_win.h"
#include "common.h"    // Helper functions, CHECK(), and CHECK_ZSTD()

BOOL compress_zstd(const char* fname, const char* outName)
{
    // fprintf(stderr, "Starting compression of %s with level 1, using 1 threads\n", fname);

    BOOL bSuccess = TRUE; // 압축 성공 여부

    /* Open the input and output files. */
    HANDLE hInput = init_file_read(fname);  // 읽을 파일
    HANDLE hOutput = init_file_write(outName);  // 쓸 파일

    // 파일 열기 오류 처리
    if (hInput == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (hOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        return FALSE;
    }

    /* Create the input and output buffers.
     * They may be any size, but we recommend using these functions to size them.
     * Performance will only suffer significantly for very tiny buffers.
     */
    size_t const buffInSize = ZSTD_CStreamInSize();
    void*  const buffIn  = malloc_orDie(buffInSize);
    size_t const buffOutSize = ZSTD_CStreamOutSize();
    void*  const buffOut = malloc_orDie(buffOutSize);

    /* Create the context. */
    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    CHECK(cctx != NULL, "ZSTD_createCCtx() failed!");

    /* Set any parameters you want.
     * Here we set the compression level, and disable the checksum.
     */
    CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, ZSTD_fast) );
    CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 0) );

    /* This loop read from the input file, compresses that entire chunk,
     * and writes all output produced to the output file.
     */
    DWORD const toRead = buffInSize;
    DWORD dwBytesRead, dwBytesWritten;
    DWORD read;
    BOOL bResult;
    OVERLAPPED readOverlap = { 0, }, writeOverlap = { 0, };  // 비동기 작업을 위한 OVERLAPPED 구조체
    for (;;) {
        bResult = async_read(
            hInput, buffIn, toRead,
            &dwBytesRead, &readOverlap, TRUE
        );
        if (bResult == FALSE) {
            fprintf(stderr, "async_read failed!\n");
            bSuccess = FALSE;
            break;  // 오류 발생 시 종료
        }
        /* Select the flush mode.
         * If the read may not be finished (read == toRead) we use
         * ZSTD_e_continue. If this is the last chunk, we use ZSTD_e_end.
         * Zstd optimizes the case where the first flush mode is ZSTD_e_end,
         * since it knows it is compressing the entire source in one pass.
         */

        read = dwBytesRead;
        readOverlap.Offset += dwBytesRead; // 읽은 만큼 오프셋 갱신

        int const lastChunk = (read < toRead);
        ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
        /* Set the input buffer to what we just read.
         * We compress until the input buffer is empty, each time flushing the
         * output.
         */
        ZSTD_inBuffer input = { buffIn, read, 0 };
        int finished;
        do {
            /* Compress into the output buffer and write all of the output to
             * the file so we can reuse the buffer next iteration.
             */
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            size_t const remaining = ZSTD_compressStream2(cctx, &output , &input, mode);
            CHECK_ZSTD(remaining);

            bResult = async_write(
                hOutput, buffOut, output.pos,
                &dwBytesWritten, &writeOverlap, TRUE
            );
            if (bResult == FALSE) {
                fprintf(stderr, "async_write failed!\n");
                bSuccess = FALSE;
                break;  // 오류 발생 시 종료
            }
            /* If we're on the last chunk we're finished when zstd returns 0,
             * which means its consumed all the input AND finished the frame.
             * Otherwise, we're finished when we've consumed all the input.
             */
            finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
            writeOverlap.Offset += dwBytesWritten; // 쓴 만큼 오프셋 갱신
        } while (!finished);
        CHECK(input.pos == input.size,
              "Impossible: zstd only returns 0 when the input is completely consumed!");

        if (!bSuccess || lastChunk) {
            break;
        }
    }

    ZSTD_freeCCtx(cctx);
    CloseHandle(hInput);
    CloseHandle(hOutput);
    free(buffIn);
    free(buffOut);

    return bSuccess;
}