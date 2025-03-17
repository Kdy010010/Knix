#include "network.h"
#include "kprint.h"
#include "type.h"

/*=========================*/
/* 8. NE2000 NIC & 간단 네트워킹 스택 */
/*=========================*/
void ne2k_init() {
    outb(NE2K_IO_BASE + NE2K_CR, CR_STP);
    outb(NE2K_IO_BASE + NE2K_PSTART, NE2K_RXBUF_START);
    outb(NE2K_IO_BASE + NE2K_PSTOP, NE2K_RXBUF_STOP);
    outb(NE2K_IO_BASE + NE2K_TPSR, NE2K_TX_PAGE);
    outb(NE2K_IO_BASE + NE2K_CR, CR_STA);
}

int ne2k_send(const uint8 *buf, uint16 len) {
    outb(NE2K_IO_BASE + NE2K_TBCR0, len & 0xFF);
    outb(NE2K_IO_BASE + NE2K_TBCR1, (len >> 8) & 0xFF);
    /* 전송 버퍼에 데이터 복사 (실제 구현에서는 DMA 필요) - 생략 */
    outb(NE2K_IO_BASE + NE2K_CR, CR_STA | 0x08); // 0x08: 가상 전송 명령
    return 0;
}

int ne2k_recv(uint8 *buf, uint16 len) {
    uint16 i;
    for (i = 0; i < len; i++) {
        buf[i] = inb(NE2K_IO_BASE + 0x10);  // 가상의 데이터 포트
    }
    return len;
}

void network_stack_init() {
    ne2k_init();
    kprint("NE2000 NIC 초기화 완료.\n");
}

void network_stack_poll() {
    uint8 packet[1500];
    int received = ne2k_recv(packet, sizeof(packet));
    if (received > 0) {
        kprint("패킷 수신됨, 크기: ");
        char numbuf[16];
        simple_itoa(received, numbuf);
        kprint(numbuf);
        kprint(" bytes\n");
        // 이후 Ethernet 프레임 처리 (ARP, IP 등) 확장 가능
    }
}

/* 네트워킹 명령어: netinfo, nettest, netapp */
void netinfo_cmd() {
    kprint("NE2000 NIC 정보:\n");
    kprint("Base I/O: 0x300\n");
    kprint("RX 버퍼: 0x40 ~ 0x80\n");
    kprint("TX 페이지: 0x20\n");
}

void nettest_cmd() {
    uint8 test_packet[64];
    uint16 i;
    for (i = 0; i < 64; i++) {
        test_packet[i] = i;
    }
    ne2k_send(test_packet, 64);
    kprint("테스트 패킷 전송 완료.\n");
}

void netapp_cmd() {
    kprint("네트워킹 응용프로그램 실행 중...\n");
    network_stack_poll();
    kprint("응용프로그램 종료.\n");
}

