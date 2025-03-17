#include "command.h"
#include "kprint.h"
#include "type.h"
#include "file.h"
#include "table.h"
#include "usb.h"
#include "process.h"
#include "network.h"
#include "system.h"

/*=========================*/
/* 12. CLI Command Processing */
/*=========================*/
void process_command(const char *cmd) {
    char tokens[10][MAX_CMD_LEN];
    int token_count = tokenize(cmd, tokens, 10);
    if (token_count == 0) return;

    if (strcmp(tokens[0], "help") == 0) {
        kprint("Commands:\n");
        kprint("  help               - 도움말\n");
        kprint("  ls [-l]            - 파일 목록\n");
        kprint("  cat <file>         - 파일 내용 출력\n");
        kprint("  write <file> <msg> - 파일 생성/업데이트\n");
        kprint("  cp <src> <dst>     - 파일 복사\n");
        kprint("  mv <src> <dst>     - 파일 이동/이름 변경\n");
        kprint("  rm <file>          - 파일 삭제\n");
        kprint("  chmod <file> <mode>- 파일 권한 변경\n");
        kprint("  chown <file> <uid> - 파일 소유자 변경\n");
        kprint("  stat <file>        - 파일 정보 출력\n");
        kprint("  touch <file>       - 빈 파일 생성\n");
        kprint("  append <file> <msg>- 파일에 내용 추가\n");
        kprint("  df                 - 남은 디스크 블록 수\n");
        kprint("  usb                - USB 장치 상태\n");
        kprint("  exec <file>        - 스크립트 실행\n");
        kprint("  execbin <file>     - 바이너리 실행 (ELF 확장 지원)\n");
        kprint("  edit <file>        - 텍스트 편집기\n");
        kprint("  find <pattern>     - 파일 검색\n");
        kprint("  sysinfo            - 시스템 정보\n");
        kprint("  fork <bin>         - 바이너리 파일로 프로세스 생성\n");
        kprint("  schedule           - 프로세스 스케줄러 실행\n");
        kprint("  netinfo            - 네트워크 정보 출력\n");
        kprint("  nettest            - 테스트 패킷 전송\n");
        kprint("  netapp             - 네트워킹 응용프로그램 실행\n");
        kprint("  reboot             - 시스템 재부팅\n");
        kprint("  shutdown           - 시스템 종료\n");
        kprint("  exit               - CLI 종료\n");
    }
    else if (strcmp(tokens[0], "ls") == 0) {
        uint32 i;
        if (token_count > 1 && strcmp(tokens[1], "-l") == 0) {
            for (i = 0; i < MAX_FILES; i++) {
                if (file_table[i].in_use) {
                    kprint(file_table[i].name); kprint("\t");
                    char numbuf[16];
                    simple_itoa(file_table[i].inode.size, numbuf); kprint(numbuf); kprint(" bytes\tMode: ");
                    simple_itoa(file_table[i].mode, numbuf); kprint(numbuf); kprint("\tOwner: ");
                    simple_itoa(file_table[i].owner, numbuf); kprint(numbuf); kprint("\n");
                }
            }
        } else {
            for (i = 0; i < MAX_FILES; i++) {
                if (file_table[i].in_use) { kprint(file_table[i].name); kprint("\n"); }
            }
        }
    }
    else if (strcmp(tokens[0], "cat") == 0) {
        if (token_count < 2) { kprint("Usage: cat <filename>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) == 0) {
            uint32 size = file_table[idx].inode.size;
            if (size < sizeof(buffer)) buffer[size] = '\0'; else buffer[sizeof(buffer)-1] = '\0';
            kprint((const char*)buffer); kprint("\n");
        } else { kprint("파일 읽기 오류.\n"); }
    }
    else if (strcmp(tokens[0], "write") == 0) {
        if (token_count < 3) { kprint("Usage: write <filename> <message>\n"); return; }
        char msg[256]; uint32 i, pos = 0;
        for (i = 2; i < (uint32)token_count; i++) {
            uint32 j = 0;
            while(tokens[i][j] && pos < sizeof(msg) - 2) { msg[pos++] = tokens[i][j++]; }
            if (i < (uint32)token_count - 1) { msg[pos++] = ' '; }
        }
        msg[pos] = '\0';
        if (find_file_index(tokens[1]) != -1) {
            if (update_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) == 0)
                kprint("파일 업데이트 성공.\n");
            else
                kprint("파일 업데이트 오류.\n");
        } else {
            if (create_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) != -1)
                kprint("파일 생성 성공.\n");
            else
                kprint("파일 생성 오류.\n");
        }
    }
    else if (strcmp(tokens[0], "cp") == 0) {
        if (token_count < 3) { kprint("Usage: cp <src> <dst>\n"); return; }
        if (copy_file(tokens[1], tokens[2]) != -1)
            kprint("파일 복사 성공.\n");
        else
            kprint("파일 복사 오류.\n");
    }
    else if (strcmp(tokens[0], "mv") == 0) {
        if (token_count < 3) { kprint("Usage: mv <src> <dst>\n"); return; }
        if (rename_file(tokens[1], tokens[2]) == 0)
            kprint("파일 이름 변경 성공.\n");
        else
            kprint("파일 이름 변경 오류.\n");
    }
    else if (strcmp(tokens[0], "rm") == 0) {
        if (token_count < 2) { kprint("Usage: rm <filename>\n"); return; }
        if (delete_file(tokens[1]) == 0)
            kprint("파일 삭제 성공.\n");
        else
            kprint("파일 삭제 오류.\n");
    }
    else if (strcmp(tokens[0], "chmod") == 0) {
        if (token_count < 3) { kprint("Usage: chmod <filename> <mode>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        file_table[idx].mode = simple_atoi(tokens[2]);
        save_file_table();
        kprint("권한 변경 완료.\n");
    }
    else if (strcmp(tokens[0], "chown") == 0) {
        if (token_count < 3) { kprint("Usage: chown <filename> <owner>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        file_table[idx].owner = simple_atoi(tokens[2]);
        save_file_table();
        kprint("소유자 변경 완료.\n");
    }
    else if (strcmp(tokens[0], "stat") == 0) {
        if (token_count < 2) { kprint("Usage: stat <filename>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        kprint("Name: "); kprint(file_table[idx].name); kprint("\nSize: ");
        char numbuf[16];
        simple_itoa(file_table[idx].inode.size, numbuf); kprint(numbuf); kprint(" bytes\nHash: ");
        simple_itoa(file_table[idx].inode.hash, numbuf); kprint(numbuf); kprint("\nMode: ");
        simple_itoa(file_table[idx].mode, numbuf); kprint(numbuf); kprint("\nOwner: ");
        simple_itoa(file_table[idx].owner, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "touch") == 0) {
        if (token_count < 2) { kprint("Usage: touch <filename>\n"); return; }
        if (find_file_index(tokens[1]) == -1) {
            if (create_file(tokens[1], (const uint8*)"", 0) != -1)
                kprint("파일 생성(터치) 성공.\n");
            else
                kprint("터치 오류.\n");
        } else {
            kprint("파일이 이미 존재합니다.\n");
        }
    }
    else if (strcmp(tokens[0], "append") == 0) {
        if (token_count < 3) { kprint("Usage: append <filename> <message>\n"); return; }
        char msg[256]; uint32 i, pos = 0;
        for (i = 2; i < (uint32)token_count; i++) {
            uint32 j = 0;
            while(tokens[i][j] && pos < sizeof(msg)-2) { msg[pos++] = tokens[i][j++]; }
            if (i < (uint32)token_count - 1) { msg[pos++] = ' '; }
        }
        msg[pos] = '\0';
        if (append_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) == 0)
            kprint("내용 추가 성공.\n");
        else
            kprint("내용 추가 오류.\n");
    }
    else if (strcmp(tokens[0], "df") == 0) {
        uint32 free_count = 0, i;
        for (i = 0; i < MAX_BLOCKS; i++) {
            if (fs.free_block_bitmap[i]) free_count++;
        }
        kprint("남은 블록 수: ");
        char numbuf[16];
        simple_itoa(free_count, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "usb") == 0) {
        kprint("USB 장치 수: ");
        char numbuf[16];
        simple_itoa(usb_device_count, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "exec") == 0) {
        if (token_count < 2) { kprint("Usage: exec <filename>\n"); return; }
        exec_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "execbin") == 0) {
        if (token_count < 2) { kprint("Usage: execbin <filename>\n"); return; }
        exec_binary_extended(tokens[1]);
    }
    else if (strcmp(tokens[0], "edit") == 0) {
        if (token_count < 2) { kprint("Usage: edit <filename>\n"); return; }
        edit_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "find") == 0) {
        if (token_count < 2) { kprint("Usage: find <pattern>\n"); return; }
        find_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "sysinfo") == 0) {
        sysinfo();
    }
    else if (strcmp(tokens[0], "fork") == 0) {
        if (token_count < 2) { kprint("Usage: fork <binary_file>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("바이너리 파일을 찾을 수 없습니다.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
            kprint("파일 읽기 오류.\n");
            return;
        }
        int pid = sys_create_process((void (*)())buffer);
        if (pid != -1) {
            kprint("새 프로세스 생성, PID: ");
            char numbuf[16];
            simple_itoa(pid, numbuf);
            kprint(numbuf); kprint("\n");
        } else {
            kprint("프로세스 생성 오류.\n");
        }
    }
    else if (strcmp(tokens[0], "schedule") == 0) {
        kprint("프로세스 스케줄러 실행...\n");
        schedule();
        kprint("스케줄러 종료.\n");
    }
    else if (strcmp(tokens[0], "netinfo") == 0) {
        netinfo_cmd();
    }
    else if (strcmp(tokens[0], "nettest") == 0) {
        nettest_cmd();
    }
    else if (strcmp(tokens[0], "netapp") == 0) {
        netapp_cmd();
    }
    else if (strcmp(tokens[0], "reboot") == 0) {
        reboot_system();
    }
    else if (strcmp(tokens[0], "shutdown") == 0) {
        shutdown_system();
    }
    else if (strcmp(tokens[0], "exit") == 0) {
        kprint("CLI 종료...\n");
        while (1);
    }
    else {
        kprint("알 수 없는 명령어입니다. 'help'를 입력하세요.\n");
    }
}