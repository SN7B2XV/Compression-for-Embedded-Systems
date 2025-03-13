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

#include "asyncio_win.h"
#include "utility.h"

/**
 * @brief 파일 읽기 초기화
 * 
 * 주어진 파일 경로에 대해 파일을 열고, 파일 크기와 청크 수를 계산합니다.
 * 
 * @param filePath 읽을 파일 경로
 * @return HANDLE 읽기 작업을 위한 파일 핸들
 */
HANDLE init_file_read(const TCHAR* filePath) {
    HANDLE hFile = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    // 파일 열기 (읽기 전용 모드, 비동기식 I/O 작업을 위한 FILE_FLAG_OVERLAPPED 설정)
    if (hFile == INVALID_HANDLE_VALUE) {
        log_message("Failed to open file for reading.");
        return INVALID_HANDLE_VALUE;
    }

    // log_message("File context for reading initialized.");
    return hFile;
}

/**
 * @brief 파일 쓰기 초기화
 * 
 * 주어진 파일 경로에 대해 파일을 쓰기 위해 파일을 엽니다.
 * 
 * @param filePath 쓸 파일 경로
 * @return HANDLE 쓰기 작업을 위한 파일 핸들
 */
HANDLE init_file_write(const TCHAR* filePath) {
    HANDLE hFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    // 파일 열기 (쓰기 전용 모드, 파일이 없으면 새로 생성, 비동기식 I/O 작업을 위한 FILE_FLAG_OVERLAPPED 설정)
    if (hFile == INVALID_HANDLE_VALUE) {
        log_message("Failed to open file for writing.");
        return INVALID_HANDLE_VALUE;
    }

    // log_message("File context for writing initialized.");
    return hFile;
}

/**
* @brief 비동기적으로 파일에서 데이터를 읽음
* @param hFile 읽을 파일의 핸들
* @param lpBuffer 데이터를 읽어들일 버퍼
* @param dwBytesToRead 읽을 데이터의 크기
* @param lpBytesRead 실제로 읽은 데이터의 크기
* @param lpOverlap OVERLAPPED 구조체 포인터 (비동기 작업을 위한 상태 정보)
* @param bWait 작업 완료 대기 여부 (TRUE: 대기, FALSE: 바로 리턴)
* @return 읽기 작업 성공 여부
*/
BOOL async_read(
    HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead,
    LPDWORD lpBytesRead, LPOVERLAPPED lpOverlap, BOOL bWait
) {
    BOOL bResult = ReadFile(hFile, lpBuffer, dwBytesToRead, lpBytesRead, lpOverlap);  // 파일 읽기 시도
    
    // I/O 작업이 대기 상태가 아니면 실패로 간주
    if (!bResult && GetLastError() != ERROR_IO_PENDING) {
        log_message("Read operation failed.");
        return FALSE;
    }

    // 작업 결과 얻기 (대기 플래그가 TRUE이면 작업 완료를 기다림)
    bResult = GetOverlappedResult(hFile, lpOverlap, lpBytesRead, bWait);  // 비동기 작업 결과 확인
    if (bWait && !bResult) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_IO_INCOMPLETE) {
            log_message("Read - GetOverlappedResult IO pending.");
            Sleep(100);
        }
    }

    // EOF (End of File) 처리: 읽은 바이트가 0이면 파일 끝에 도달한 것
    if (*lpBytesRead == 0) {
        log_message("EOF reached.");
    }

    return TRUE;
}

/**
* @brief 비동기적으로 파일에 데이터를 씀
* @param hFile 쓸 파일의 핸들
* @param lpBuffer 쓸 데이터를 담고 있는 버퍼
* @param dwBytesToWrite 쓸 데이터의 크기
* @param lpBytesWritten 실제로 쓴 데이터의 크기
* @param lpOverlap OVERLAPPED 구조체 포인터 (비동기 작업을 위한 상태 정보)
* @param bWait 작업 완료 대기 여부 (TRUE: 대기, FALSE: 바로 리턴)
* @return 쓰기 작업 성공 여부
*/
BOOL async_write(
    HANDLE hFile, LPCVOID lpBuffer, DWORD dwBytesToWrite,
    LPDWORD lpBytesWritten, LPOVERLAPPED lpOverlap, BOOL bWait
) {
    BOOL bResult = WriteFile(hFile, lpBuffer, dwBytesToWrite, lpBytesWritten, lpOverlap);  // 파일 쓰기 시도

    // I/O 작업이 대기 상태가 아니면 실패로 간주
    if (!bResult && GetLastError() != ERROR_IO_PENDING) {
        log_message("Write operation failed.");
        return FALSE;
    }

    // 작업 결과 얻기 (대기 플래그가 TRUE이면 작업 완료를 기다림)
    bResult = GetOverlappedResult(hFile, lpOverlap, lpBytesWritten, bWait);  // 비동기 작업 결과 확인
    if (bWait && !bResult) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_IO_INCOMPLETE) {
            log_message("Wrtie - GetOverlappedResult IO pending.");
            Sleep(100);
        }
    }

    return TRUE;
}
