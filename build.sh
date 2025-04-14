#!/bin/bash

# Compiler and Assembler
CC=gcc
ASM=nasm
LD=ld

# Compiler and Linker flags
CFLAGS="-m32 -ffreestanding -Wall -Wextra -nostdlib -nostartfiles -no-pie"
LDFLAGS="-m elf_i386"

# Source and Build directories
SRC_DIR="src"
BUILD_DIR="build"
ISO_DIR="iso"

# Kernel source files
KERNEL_SRC=(
    "$SRC_DIR/kernel/kernel.c"
    "$SRC_DIR/kernel/kprint.c"
    "$SRC_DIR/kernel/network.c"
    "$SRC_DIR/kernel/command.c"
    "$SRC_DIR/kernel/disk.c"
    "$SRC_DIR/kernel/file.c"
    "$SRC_DIR/kernel/process.c"
    "$SRC_DIR/kernel/type.c"
    "$SRC_DIR/kernel/table.c"
    "$SRC_DIR/kernel/usb.c"
    "$SRC_DIR/kernel/system.c"
)

# Kernel object files
KERNEL_OBJ=()
for src in "${KERNEL_SRC[@]}"; do
    obj="$BUILD_DIR/$(basename ${src%.c}.o)"
    KERNEL_OBJ+=("$obj")
done

# Bootloader source and object files
BOOTLOADER_SRC="$SRC_DIR/bootloader.asm"
BOOTLOADER_OBJ="$BUILD_DIR/bootloader.o"
BOOTLOADER_BIN="$BUILD_DIR/bootloader.bin"
KERNEL_ELF="$BUILD_DIR/kernel.elf"
KERNEL_BIN="$BUILD_DIR/kernel.bin"
ISO_IMAGE="$BUILD_DIR/knix.iso"

# Create build directory
mkdir -p "$BUILD_DIR"
mkdir -p "$ISO_DIR"

# Compile bootloader
echo "Assembling bootloader..."
$ASM -f bin "$BOOTLOADER_SRC" -o "$BOOTLOADER_BIN"

# Compile each kernel C file
echo "Compiling kernel source files..."
for src in "${KERNEL_SRC[@]}"; do
    obj="$BUILD_DIR/$(basename ${src%.c}.o)"
    echo "Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
done

# Link kernel ELF (for debugging)
echo "Linking kernel ELF..."
$LD $LDFLAGS -T linker.ld -o "$KERNEL_ELF" "${KERNEL_OBJ[@]}"

# Convert ELF to binary
echo "Converting kernel ELF to binary..."
objcopy -O binary "$KERNEL_ELF" "$KERNEL_BIN"

# Ensure kernel is correctly linked
if [[ ! -f "$KERNEL_BIN" ]]; then
    echo "Error: Kernel binary not created!"
    exit 1
fi

echo "Build complete: $KERNEL_BIN"

# Create ISO
echo "Creating ISO image..."
mkdir -p "$ISO_DIR/boot"
cp "$BOOTLOADER_BIN" "$ISO_DIR/bootloader.bin"
cp "$KERNEL_BIN" "$ISO_DIR/kernel.bin"

# mkisofs 또는 genisoimage 사용하여 부팅 가능한 ISO 생성
mkisofs -R -b bootloader.bin -no-emul-boot -o "$ISO_IMAGE" "$ISO_DIR"

echo "ISO created: $ISO_IMAGE"

# Clean function
if [[ $1 == "clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi
