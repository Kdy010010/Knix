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
        kprint("  help               - Show help\n");
        kprint("  ls [-l]            - List files\n");
        kprint("  cat <file>         - Display file contents\n");
        kprint("  write <file> <msg> - Create/update a file\n");
        kprint("  cp <src> <dst>     - Copy a file\n");
        kprint("  mv <src> <dst>     - Move/rename a file\n");
        kprint("  rm <file>          - Delete a file\n");
        kprint("  chmod <file> <mode>- Change file permissions\n");
        kprint("  chown <file> <uid> - Change file owner\n");
        kprint("  stat <file>        - Display file information\n");
        kprint("  touch <file>       - Create an empty file\n");
        kprint("  append <file> <msg>- Append content to a file\n");
        kprint("  df                 - Show available disk blocks\n");
        kprint("  usb                - Display USB device status\n");
        kprint("  exec <file>        - Execute a script\n");
        kprint("  execbin <file>     - Execute a binary (supports ELF format)\n");
        kprint("  edit <file>        - Open text editor\n");
        kprint("  find <pattern>     - Search for files\n");
        kprint("  sysinfo            - Display system information\n");
        kprint("  fork <bin>         - Create a process from a binary file\n");
        kprint("  schedule           - Run process scheduler\n");
        kprint("  netinfo            - Display network information\n");
        kprint("  nettest            - Send test packets\n");
        kprint("  netapp             - Run a networking application\n");
        kprint("  reboot             - Reboot the system\n");
        kprint("  shutdown           - Shut down the system\n");
        kprint("  exit               - Exit CLI\n");
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
        if (idx == -1) { kprint("File not found.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) == 0) {
            uint32 size = file_table[idx].inode.size;
            if (size < sizeof(buffer)) buffer[size] = '\0'; else buffer[sizeof(buffer)-1] = '\0';
            kprint((const char*)buffer); kprint("\n");
        } else { kprint("File read error.\n"); }
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
                kprint("File update successful.\n");
            else
                kprint("File update error.\n");
        } else {
            if (create_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) != -1)
                kprint("File creation successful.\n");
            else
                kprint("File creation error.\n");
        }
    }
    else if (strcmp(tokens[0], "cp") == 0) {
        if (token_count < 3) { kprint("Usage: cp <src> <dst>\n"); return; }
        if (copy_file(tokens[1], tokens[2]) != -1)
            kprint("File copy successful.\n");
        else
            kprint("File copy error.\n");
    }
    else if (strcmp(tokens[0], "mv") == 0) {
        if (token_count < 3) { kprint("Usage: mv <src> <dst>\n"); return; }
        if (rename_file(tokens[1], tokens[2]) == 0)
            kprint("File renaming successful.\n");
        else
            kprint("File renaming error.\n");
    }
    else if (strcmp(tokens[0], "rm") == 0) {
        if (token_count < 2) { kprint("Usage: rm <filename>\n"); return; }
        if (delete_file(tokens[1]) == 0)
            kprint("File deletion successful.\n");
        else
            kprint("File deletion error.\n");
    }
    else if (strcmp(tokens[0], "chmod") == 0) {
        if (token_count < 3) { kprint("Usage: chmod <filename> <mode>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("File not found.\n"); return; }
        file_table[idx].mode = simple_atoi(tokens[2]);
        save_file_table();
        kprint("Permission change completed.\n");
    }
    else if (strcmp(tokens[0], "chown") == 0) {
        if (token_count < 3) { kprint("Usage: chown <filename> <owner>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("File not found.\n"); return; }
        file_table[idx].owner = simple_atoi(tokens[2]);
        save_file_table();
        kprint("Owner change completed.\n");
    }
    else if (strcmp(tokens[0], "stat") == 0) {
        if (token_count < 2) { kprint("Usage: stat <filename>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("File not found.\n"); return; }
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
                kprint("File creation (touch) successful.\n");
            else
                kprint("Touch error.\n");
        } else {
            kprint("The file already exists.\n");
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
            kprint("Content addition successful.\n");
        else
            kprint("Error adding content.\n");
    }
    else if (strcmp(tokens[0], "df") == 0) {
        uint32 free_count = 0, i;
        for (i = 0; i < MAX_BLOCKS; i++) {
            if (fs.free_block_bitmap[i]) free_count++;
        }
        kprint("Number of blocks remaining: ");
        char numbuf[16];
        simple_itoa(free_count, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "usb") == 0) {
        kprint("Number of USB devices: ");
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
        if (idx == -1) { kprint("Binary file not found.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
            kprint("File Read Error.\n");
            return;
        }
        int pid = sys_create_process((void (*)())buffer);
        if (pid != -1) {
            kprint("Create a new process, PID: ");
            char numbuf[16];
            simple_itoa(pid, numbuf);
            kprint(numbuf); kprint("\n");
        } else {
            kprint("Process Creation Error.\n");
        }
    }
    else if (strcmp(tokens[0], "schedule") == 0) {
        kprint("Run Process Scheduler...\n");
        schedule();
        kprint("Shutting down the scheduler.\n");
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
        kprint("Shutting down the CLI...\n");
        while (1);
    }
    else {
        kprint("Unknown command. Please type 'help'.\n");
    }
}