#include "system.h"
#include "kprint.h"
#include "io.h"


void sysinfo() {
    kprint("Knix OS - System Info\n");
    kprint("Version: 0.3\n");
    kprint("Author: Unknown\n");
}

void reboot_system() {
    kprint("Rebooting system...\n");

    // x86 아키텍처에서 키보드 컨트롤러를 이용한 소프트 리부트
    unsigned char good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);  // 키보드 컨트롤러 상태 확인
    }
    outb(0x64, 0xFE);  // 재부팅 명령 전송
    while (1) {}  // 재부팅 실패 시 무한 루프
}

void shutdown_system() {
    kprint("Shutting down system...\n");

    // ACPI를 통한 시스템 종료 (x86 환경에서 사용 가능)
    outw(0xB004, 0x2000);  // Bochs, QEMU에서 동작
    outw(0x604, 0x2000);   // 일부 x86 시스템에서 동작
    outw(0x4004, 0x3400);  // VirtualBox에서 동작

    while (1) {}  // 종료되지 않으면 무한 루프
}
