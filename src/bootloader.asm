; bootloader.asm - Knix OS용 간단한 부트로더 예제
; NASM으로 컴파일: nasm -f bin bootloader.asm -o bootloader.bin

[BITS 16]
[ORG 0x7C00]      ; 부트로더 로드 주소 명시

section .text
global _start
_start:
    ; BIOS가 전달한 부트 드라이브 번호(DL)를 메모리에 저장
    mov [boot_drive], dl

    ; 텍스트 모드 설정 및 화면 클리어
    mov ax, 0x0003
    int 0x10

    ; 부트 메시지 출력
    mov si, boot_msg
    call print_string

    ; 커널 로드: 디스크의 섹터 2부터 36개 섹터를 메모리 0x8000에 로드
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000 ; ES:BX = 0x0000:0x8000
    mov ah, 0x02    ; 읽기 기능
    mov al, 36      ; 섹터 수
    mov ch, 0       ; 실린더 0
    mov dh, 0       ; 헤드 0
    mov cl, 2       ; 시작 섹터 (2번)
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000  ; 커널 실행 (성공 시)

disk_error:
    mov si, err_msg
    call print_string
    mov ah, 0x0E
    mov al, 'E'  ; 오류 코드 표시
    int 0x10
    mov al, 'R'
    int 0x10
    mov al, ':'
    int 0x10

    ; AH 값을 16진수로 출력하는 코드 추가
    mov al, ah
    call print_hex
    jmp $

print_hex:
    push ax
    push bx
    push cx
    push dx

    mov bx, ax       ; 오류 코드(AH)를 BX에 저장
    shr bx, 4        ; 상위 4비트를 얻음
    call print_nibble

    mov bx, ax       ; 다시 원래 값으로 복원
    and bx, 0x0F     ; 하위 4비트만 남김
    call print_nibble

    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_nibble:
    add bx, '0'      ; 숫자로 변환
    cmp bx, '9'
    jbe .print
    add bx, 7        ; A~F 처리

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

; 데이터 섹션 (같은 .text 섹션 내에 배치)
boot_msg  db "Loading Knix OS kernel...", 0
err_msg   db "Disk read error!", 0
boot_drive db 0

; 512바이트 패딩 및 부트 시그니처
times 510 - ($ - $$) db 0
dw 0xAA55