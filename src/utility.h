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

#ifndef UTILITY_H
#define UTILITY_H

#include <windows.h>
#include <stdio.h>
#include "compressor.h"

// 함수 선언

void log_message(const TCHAR* message);
DWORD get_file_size(HANDLE hFile);
const TCHAR* get_extension(CompressionAlgorithm algorithm);
TCHAR* get_output_file_name(const TCHAR* filename, CompressionAlgorithm algorithm);

#endif // UTILITY_H
