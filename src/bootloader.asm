; bootloader.asm - Knix OS용 간단한 부트로더 예제
; NASM으로 컴파일: nasm -f bin bootloader.asm -o bootloader.bin

[BITS 16]

global _start
section .text
_start:
    ; BIOS가 전달한 부트 드라이브 번호(DL)를 메모리에 저장
    mov [boot_drive], dl

    ; (선택 사항) 텍스트 모드 설정 및 화면 클리어
    mov ax, 0x0003
    int 0x10

    ; 부트 메시지 출력
    mov si, boot_msg
    call print_string

    ; 커널 로드: 디스크의 섹터 2부터 36개 섹터를 메모리 0x8000에 로드
    mov ax, 0x0000
    mov es, ax    ; 🔥 ES를 0으로 설정 (0x0000:0x8000)
    mov bx, 0x8000 ; 커널 로드 주소
    mov di, bx     ; 로드할 메모리 포인터
    mov dh, 0      ; 헤드 0
    mov ch, 0      ; 실린더 0
    mov cl, 2      ; 시작 섹터 번호 (2번 섹터부터 시작)
    mov dl, [boot_drive] ; 부트 드라이브 번호

    mov cx, 36     ; 🔥 18KB 커널을 모두 로드하기 위해 36개 섹터 읽기
read_sector:
    push cx
    mov ah, 0x02  ; BIOS 디스크 읽기 함수
    mov al, 1     ; 한 번에 1 섹터씩 읽음
    mov bx, di    ; 로드할 메모리 주소
    int 0x13      ; BIOS 인터럽트 호출
    jc disk_error ; 에러 발생 시 disk_error 루틴으로 이동
    add di, 512   ; 1 섹터(512바이트) 만큼 다음 메모리 주소로 이동
    inc cl        ; 다음 섹터 번호 (단순 증가)
    pop cx
    dec cx
    jnz read_sector

    ; 커널 로드 완료 후, 정확한 Entry Point로 점프 (0x0000:0x8114)
    jmp 0x0000:0x8114  ; 🔥 Entry Point로 점프

disk_error:
    mov si, err_msg
    call print_string
    jmp $

;-------------------------------------------------------
; 간단한 문자열 출력 서브루틴 (BIOS teletype, AH=0x0E)
; SI가 문자열 시작 주소를 가리킵니다. 문자열은 0으로 종료됩니다.
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
;-------------------------------------------------------

boot_msg  db "Loading Knix OS kernel...", 0
err_msg   db "Disk read error!", 0
boot_drive db 0

; 부트 섹터는 512바이트여야 하므로 남은 부분을 0으로 채움
times 510 - ($ - $$) db 0
dw 0xAA55           ; 부트 섹터 시그니처
