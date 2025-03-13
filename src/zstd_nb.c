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

#include "zstd_nb.h"
#include "asyncio_win.h"
#include "utility.h"

BOOL create_resources(resources_t** ress)
{
    *ress = (resources_t*)calloc(1, sizeof(resources_t));
    (*ress)->srcBufMaxSize = ZSTD_CStreamInSize();   /* can always read one full block */
    (*ress)->dstBufMaxSize = ZSTD_CStreamOutSize();  /* can always flush a full block */
    (*ress)->srcBuf = malloc((*ress)->srcBufMaxSize);
    (*ress)->dstBuf= malloc((*ress)->dstBufMaxSize);

    /* Create the context. */
    (*ress)->cctxPtr = ZSTD_createCCtx();

    /* Set any compression parameters you want here.
     * They will persist for every compression operation.
     * Here we set the compression level, and disable the checksum.
     */
    size_t const zstdSetLevelResult = ZSTD_CCtx_setParameter((*ress)->cctxPtr, ZSTD_c_compressionLevel, ZSTD_fast);
    size_t const zstdSetCheckSumResult = ZSTD_CCtx_setParameter((*ress)->cctxPtr, ZSTD_c_checksumFlag, 1);
    
    if ((*ress)->cctxPtr != NULL && (*ress)->srcBuf && (*ress)->dstBuf &&
        !ZSTD_isError(zstdSetLevelResult) && !ZSTD_isError(zstdSetCheckSumResult)
    ) { 
        return TRUE;
    }

    free_resources(*ress);
    *ress = NULL;

    return FALSE;
}

void free_resources(resources_t* ress)
{
    if (ress == NULL) {
         return;
    }

    ZSTD_freeCCtx(ress->cctxPtr);
    free(ress->srcBuf);
    free(ress->dstBuf);
}

BOOL ZSTD_NB_Process(resources_t* ress, HANDLE hInput, HANDLE hOutput)
{
    BOOL bResult = TRUE;
    BOOL bFirst = TRUE;
    // int cnt = 0;

    /* This loop read from the input file, compresses that entire chunk,
     * and writes all output produced to the output file.
     */
    BOOL bAsyncResult;
    DWORD const toRead = ress->srcBufMaxSize;
    DWORD dwRead, dwBytesRead, dwBytesWritten;
    OVERLAPPED readOverlap = { 0, }, writeOverlap = { 0, }; // OVERLAPPED structure for asynchronous operations
    for (;;) {
        bAsyncResult = async_read(
            hInput, ress->srcBuf, toRead,
            &dwBytesRead, &readOverlap, TRUE
        );
        if (bAsyncResult == FALSE) {
            log_message("async_read failed!");
            bResult = FALSE;
            break; // Exit on error
        }
        /* Select the flush mode.
         * If the read may not be finished (read == toRead) we use
         * ZSTD_e_continue. If this is the last chunk, we use ZSTD_e_end.
         * Zstd optimizes the case where the first flush mode is ZSTD_e_end,
         * since it knows it is compressing the entire source in one pass.
         */

        dwRead = dwBytesRead;
        readOverlap.Offset += dwBytesRead; // Update offset by amount read

        int const lastChunk = (dwRead < toRead);
        ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
        /* Set the input buffer to what we just read.
         * We compress until the input buffer is empty, each time flushing the
         * output.
         */
        ZSTD_inBuffer input = { ress->srcBuf, dwRead, 0 };
        int finished;
        do {
            /* Compress into the output buffer and write all of the output to
             * the file so we can reuse the buffer next iteration.
             */
            ZSTD_outBuffer output = { ress->dstBuf, ress->dstBufMaxSize, 0 };
            size_t const remaining = ZSTD_compressStream2(ress->cctxPtr, &output, &input, mode);
            if (ZSTD_isError(remaining)) {
                log_message("ZSTD Compress Stream failed!");
            }

            if (!bFirst) {
                // Check the result of the asynchronous operation
                bResult = GetOverlappedResult(hOutput, &writeOverlap, &dwBytesWritten, TRUE);
                writeOverlap.Offset += dwBytesWritten; // Update offset by amount written
            }

            bAsyncResult = async_write(
                hOutput, ress->dstBuf, output.pos,
                &dwBytesWritten, &writeOverlap, FALSE
            );
            if (bAsyncResult == FALSE) {
                log_message("async_write failed!");
                bResult = FALSE;
                break; // Exit on error
            }
            /* If we're on the last chunk we're finished when zstd returns 0,
             * which means its consumed all the input AND finished the frame.
             * Otherwise, we're finished when we've consumed all the input.
             */
            finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
            if (bFirst) {
                bFirst = FALSE;
            }
        } while (!finished);

        if (input.pos != input.size) {
            bResult = FALSE;
            log_message("Impossible: zstd only returns 0 when the input is completely consumed!");
            break;
        }
        
        if (!bResult || lastChunk) {
            break;
        }

        // Preventing Kernel Resource Saturation
        // if ((cnt % 16) == 15) {
        //     Sleep(10);
        // }
        // cnt++;
    }

    return bResult;
}

BOOL compress_zstd(const TCHAR* fname, const TCHAR* outName)
{
    // log_message("Starting compression of %s with level 1, using 1 threads", fname);

    BOOL bResult = TRUE; // Compression success status

    /* Open the input and output files. */
    HANDLE hInput = init_file_read(fname); // File to read
    HANDLE hOutput = init_file_write(outName); // File to write

    resources_t* ress;

    // Handle file opening errors
    if (hInput == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (hOutput == INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        return FALSE;
    }

    if (create_resources(&ress)) {
        bResult = ZSTD_NB_Process(ress, hInput, hOutput);
    } else {
        log_message("error : ZSTD resource allocation failed.");
    }

    // Cleanup resources
    CloseHandle(hInput);
    CloseHandle(hOutput);
    free_resources(ress);

    return bResult;
}
