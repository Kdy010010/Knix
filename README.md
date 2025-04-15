# Knix

Knix는 저수준 언어와 직접 제작한 툴체인을 이용해 개발된 OS 커널 프로젝트입니다.
C와 Assembly로 구성된 커널과 cslash라는 독자 스크립트 언어를 통해, 운영체제 내부 기능 및 유틸리티 도구들을 제작할 수 있습니다.

## 특징
* x86 기반 커널

* 부트로더 직접 작성

* 파일, 디스크, 네트워크, 프로세스, USB 등 핵심 기능 지원

* CSlash (독자 스크립트 언어) 지원

* Python 기반 cslash 파서 제공

## 빌드 방법

```bash
# 빌드 스크립트 실행
./build.sh

# GRUB 이미지 빌드
./grub-build.sh
```

빌드 완료 후 `os.iso` 파일이 생성됩니다.

## 실행 방법 (QEMU)

```bash
qemu-system-x86_64 -cdrom os.iso
```

## 프로젝트 구조

```scss
├── src/
│   ├── bootloader.asm    # 부트로더
│   └── kernel/           # 커널 소스
│   └── utils/            # cslash 관련 파일
├── cslash/main.py        # cslash 파서
├── build.sh              # 빌드 스크립트
├── grub-build.sh         # GRUB 빌드 스크립트
├── linker.ld             # 링커 스크립트
└── README.md
```

## 라이선스

[Apache License](LICENSE)

