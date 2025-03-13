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
#include "compressor.h"
#include <time.h> // 소요 시간 확인용

#define STRINGIFY(x) #x

#define INPUT_FILE "../sample_files/input.txt"

void check_compress_time(CompressionAlgorithm algorithm, const TCHAR* name) {
    clock_t start, end;
    double time_spent;
    
    // 충분한 크기의 배열로 선언
    TCHAR msg[50];
    TCHAR* const output = get_output_file_name(INPUT_FILE, algorithm);

    start = clock(); // 시작 시간 기록

    compress_file(INPUT_FILE, output, algorithm); // 파일 압축
    
    end = clock(); // 종료 시간 기록
    time_spent = (double)(end - start) / CLOCKS_PER_SEC; // 실행 시간 계산
    sprintf(msg, "%8s took %f seconds to execute.", name, time_spent); // 결과 출력
    log_message(msg);

    free(output);
}

int main() {
    for (int i = 0; i < 3; i++) {
        check_compress_time(LZ4, STRINGIFY(LZ4));
        check_compress_time(ZSTD, STRINGIFY(ZSTD));
        log_message("");
    }
}
