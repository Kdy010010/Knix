[BITS 16]
[ORG 0x7C00]

section .text
global _start
_start:
    mov [boot_drive], dl  ; BIOS에서 전달된 부트 드라이브 번호 저장

    ; 화면 클리어
    mov ax, 0x0003
    int 0x10

    mov si, boot_msg
    call print_string

    ; CHS 모드 사용
    mov si, chs_mode_msg
    call print_string

    ; 메모리 클리어 (커널을 로드할 0x1000 영역을 0으로 초기화)
    mov cx, 512
    mov di, 0x1000
    xor ax, ax
clear_loop:
    stosw
    loop clear_loop

    ; CHS 모드로 디스크 읽기 (LBA 1을 CHS로 변환)
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000   ; ES:BX = 0x1000:0x0000
    mov ah, 0x02     ; BIOS 디스크 읽기 기능
    mov al, 36       ; 섹터 수
    mov ch, 0        ; 실린더 0
    mov dh, 0        ; 헤드 0
    mov cl, 2        ; 섹터 2부터 읽기 (LBA 1을 CHS로 변환)
    mov dl, [boot_drive]
    int 0x13
    jc disk_error    ; CHS 실패 시 오류 메시지

    ; 💡 메모리 덤프 추가 (커널이 정상적으로 로드되었는지 확인)
    mov si, mem_dump_msg
    call print_string
    mov cx, 16       ; 첫 16바이트 출력
    mov si, 0x1000   ; 커널이 로드된 메모리 주소
dump_loop:
    lodsb
    call print_hex
    loop dump_loop

    jmp 0x1000:0x0000  ; 커널 실행 (정상적으로 로드되었으면 점프)

disk_error:
    mov si, err_msg
    call print_string
    jmp $

print_hex:
    push ax
    push bx
    push cx
    push dx

    mov bx, ax
    shr bx, 4
    call print_nibble

    mov bx, ax
    and bx, 0x0F
    call print_nibble

    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_nibble:
    add bx, '0'
    cmp bx, '9'
    jbe .print
    add bx, 7

.print:
    mov al, bl
    int 0x10
    ret

print_string:
    mov ah, 0x0E
.print_loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .print_loop
.done:
    ret

; 데이터 영역
boot_msg  db "Loading Knix OS kernel...", 0
chs_mode_msg db "Using CHS mode...", 0
mem_dump_msg db "Memory dump: ", 0
err_msg   db "Disk read error!", 0
boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xAA55
