#!/bin/bash

# Compiler and Linker settings
CC=gcc
LD=ld

# Compiler and Linker flags
CFLAGS="-m32 -ffreestanding -Wall -Wextra -nostdlib -nostartfiles -no-pie"
LDFLAGS="-m elf_i386"

# Directories
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

# Kernel output files
KERNEL_ELF="$BUILD_DIR/kernel.elf"
ISO_IMAGE="$BUILD_DIR/knix.iso"

# Create directories if not exist
mkdir -p "$BUILD_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# Compile each kernel C file
echo "Compiling kernel source files..."
for src in "${KERNEL_SRC[@]}"; do
    obj="$BUILD_DIR/$(basename ${src%.c}.o)"
    echo "Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
done

# Link kernel ELF
echo "Linking kernel ELF..."
$LD $LDFLAGS -T linker.ld -o "$KERNEL_ELF" "${KERNEL_OBJ[@]}"

# Ensure kernel is correctly linked
if [[ ! -f "$KERNEL_ELF" ]]; then
    echo "Error: Kernel ELF not created!"
    exit 1
fi

echo "Kernel ELF built: $KERNEL_ELF"

# Create GRUB configuration
echo "Creating GRUB configuration..."
cat > "$ISO_DIR/boot/grub/grub.cfg" << EOF
set timeout=0
set default=0

menuentry "Knix OS" {
    set root=(hd0,1)
    multiboot /boot/kernel.bin
}
EOF

# Copy kernel ELF to ISO directory
echo "Copying kernel ELF..."
cp "$KERNEL_ELF" "$ISO_DIR/boot/kernel.elf"

# Create bootable ISO using GRUB
echo "Creating GRUB-based ISO..."
grub-mkrescue -o "$ISO_IMAGE" "$ISO_DIR"

echo "GRUB-based ISO created: $ISO_IMAGE"

# Run with QEMU for testing
if [[ $1 == "run" ]]; then
    echo "Running Knix OS in QEMU..."
    qemu-system-x86_64 -cdrom "$ISO_IMAGE"
fi

# Clean function
if [[ $1 == "clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR" "$ISO_DIR"
fi
