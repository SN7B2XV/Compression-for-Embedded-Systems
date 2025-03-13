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

#include "compressor.h"
#include "lz4nb.h"
#include "zstd_nb.h"

/**
* @brief Non-Blocking 방식으로 읽기와 쓰기 작업을 수행하고, 데이터를 압축하여 파일에 씁니다.
*
* 이 함수는 파일을 Non-Blocking 방식으로 읽고, 읽은 데이터를 압축 후 다른 파일로 Non-Blocking 방식으로 씁니다.
*
* @param inputFilePath 읽을 파일 경로
* @param outputFilePath 쓸 파일 경로
* @param algorithm 압축 알고리즘
* @return 압축 성공 여부
*/
BOOL compress_file(const TCHAR* inputFilePath, const TCHAR* outputFilePath, CompressionAlgorithm algorithm) {

    BOOL bResult = FALSE;
    switch(algorithm) {
        case LZ4:
            bResult = compress_lz4(inputFilePath, outputFilePath);
            break;
        case ZSTD:
            bResult = compress_zstd(inputFilePath, outputFilePath);
            break;
    }

    return bResult;
}
