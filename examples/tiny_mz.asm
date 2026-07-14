; tiny.asm - smallest valid MZ .EXE that prints and exits
; nasm -f bin -o tiny.exe tiny.asm
BITS 16

org 0
; MZ header
header:
    db 'M', 'Z'        ; e_magic
    dw 50              ; e_cblp (bytes on last page = total file size)
    dw 1               ; e_cp (pages)
    dw 0               ; e_crlc (relocations)
    dw 2               ; e_cparhdr (header size in paragraphs, 2*16=32 bytes)
    dw 0               ; e_minalloc
    dw 0xffff          ; e_maxalloc
    dw 0x0000          ; e_ss (initial SS, relative to load module)
    dw 0xfffe          ; e_sp
    dw 0               ; e_csum
    dw 0x0000          ; e_ip (relative to load module)
    dw 0x0000          ; e_cs (relative to load module)
    dw 0x001e          ; e_lfarlc (relocation table offset, after header)
    dw 0               ; e_ovno

; Pad to 32 bytes (2 paragraphs = 32 bytes)
times 32 - ($ - $$) db 0

; Code starts here. CS=0, IP=0 in load module
start:
    mov dx, msg
    mov ax, 0x0900
    int 0x21
    mov ax, 0x4c00
    int 0x21

msg: db 'OK', 0x0d, 0x0a, '$'