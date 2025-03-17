/*
   Knix OS: 확장형 Unix-like Desktop OS
   - Persistent KnixFS, 파일 테이블, CLI, USB 드라이버
   - 스크립트 실행(exec), 바이너리 실행(execbin: ELF 확장 지원)
   - 간단한 프로세스 관리(멀티태스킹)와 시스템 호출 인터페이스
   - NE2000 기반 간단한 네트워킹 스택 및 네트워킹 명령어(netinfo, nettest, netapp) 진짜 힘들다
*/

#include "kprint.h"
#include "file.h"
#include "table.h"
#include "usb.h"
#include "process.h"
#include "network.h"
#include "command.h"

/*=========================*/
/* 14. Kernel Main */
/*=========================*/
void kmain() {
    kprint("OK\n");
    int ret;
    char cmdline[MAX_CMD_LEN] = {0};

    ret = load_fs();
    if (ret != 0) {
        init_fs();
        ret = save_fs();
        if (ret != 0) while(1);
        kprint("새 FS 초기화 완료.\n");
    } else {
        kprint("기존 FS 로드 완료.\n");
    }

    ret = load_file_table();
    if (ret != 0) {
        init_file_table();
        ret = save_file_table();
        if (ret != 0) while(1);
        kprint("새 파일 테이블 초기화 완료.\n");
    } else {
        kprint("기존 파일 테이블 로드 완료.\n");
    }

    init_processes();

    usb_scan();
    kprint("USB 장치 스캔 완료.\n");
    usb_poll();

    network_stack_init();

    /* 메인 CLI 루프와 함께 주기적으로 네트워킹 패킷 폴링 */
    while (1) {
        kprint("knix> ");
        kgets(cmdline, MAX_CMD_LEN);
        process_command(cmdline);
        network_stack_poll();
    }

    while(1);
}

void _start() {
    kmain();
    while (1) {}
}