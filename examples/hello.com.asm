; hello.com.asm — minimal .COM that prints "Hello, world!"
; nasm -f bin -o hello.com hello.com.asm
BITS 16

org 0x100

mov dx, msg
mov ax, 0x0900
int 0x21
mov ax, 0x4c00
int 0x21

msg: db 'Hello, world!', 0x0d, 0x0a, '$'