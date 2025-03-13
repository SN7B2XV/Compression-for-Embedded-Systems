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

#include "utility.h"

/**
* @brief 로그 메시지를 출력하는 함수
* @param message 출력할 로그 메시지
*/
void log_message(const TCHAR* message) {
    printf("%s\n", message);  // @message: 출력할 로그 메시지
}

/**
 * @brief 파일 크기 얻기
 * 
 * 주어진 파일 핸들을 통해 파일 크기를 반환합니다.
 * 
 * @param hFile 파일 핸들
 * @return DWORD 파일 크기
 */
DWORD get_file_size(HANDLE hFile) {
    LARGE_INTEGER size;
    if (GetFileSizeEx(hFile, &size)) {
        return (DWORD)size.QuadPart; // 파일 크기를 DWORD 형으로 반환
    }
    return 0; // 오류 시 0 반환
}

/**
 * @brief 압축 알고리듬에 따른 확장자 반환
 * 
 * 주어진 압축 알고리듬에 대한 파일 확장자를 반환합니다.
 * 
 * @param algorithm 압축 알고리듬
 * @return 압축 알고리듬에 해당하는 확장자
 */
const TCHAR* get_extension(CompressionAlgorithm algorithm) {
    switch (algorithm) {
    case LZ4:
        return ".lz4";
    case ZSTD:
        return ".zst";
    }
    return ".zip";
}

/**
 * @brief 출력 파일 이름 생성
 * 
 * 주어진 파일 이름에 압축 알고리듬에 해당하는 확장자를 추가하여 새로운 파일 이름을 생성합니다.
 * 
 * @param filename 원본 파일 이름
 * @param algorithm 압축 알고리듬
 * @return 생성된 새로운 파일 이름
 */
TCHAR* get_output_file_name(const TCHAR* filename, CompressionAlgorithm algorithm) {
    size_t const inL = strlen(filename);
    size_t const outL = inL + 5; // 4자 (".zip") + 1자 (널 종단자)
    TCHAR* const outSpace = (TCHAR*)malloc(outL);
    memset(outSpace, 0, outL);
    strcat(outSpace, filename);
    strcat(outSpace, get_extension(algorithm));
    return (TCHAR*)outSpace;
}