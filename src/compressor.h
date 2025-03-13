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

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <windows.h>

// enum 선언

typedef enum {
    LZ4,
    ZSTD,
    ALGORITHM_COUNT // 사용 가능한 알고리듬의 수
} CompressionAlgorithm;

// 함수 선언
BOOL compress_file(const TCHAR *inputFilePath, const TCHAR *outputFilePath, CompressionAlgorithm algorithm);

#endif // COMPRESSOR_H
