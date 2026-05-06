[bits 32]

section .multiboot_header
align 8
header_start:
    dd 0xe85250d6                ; Multiboot 2 magic number
    dd 0                         ; Mimari 0 (i386 protected mode)
    dd header_end - header_start ; Header uzunluğu
    ; Checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; End Tag
    dw 0    
    dw 0    
    dd 8    
header_end:

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    mov esp, stack_top

    ; --- 1. Paging Hazırlığı (Identity Mapping) ---
    mov eax, p3_table
    or eax, 0b11 ; present + writable
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11 ; present + writable
    mov [p3_table], eax

    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000 ; 2MiB
    mul ecx
    or eax, 0b10000011 ; present + writable + huge page (bit 7)
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ; --- 2. Long Mode Aktif Etme ---
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5 ; PAE (Physical Address Extension)
    mov cr4, eax

    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or eax, 1 << 8      ; LME (Long Mode Enable)
    wrmsr

    mov eax, cr0
    or eax, 1 << 31     ; PG (Paging)
    mov cr0, eax

    ; --- 3. GDT ve 64-bit'e Zıplama ---
    lgdt [gdt64.pointer]
    
    ; Far jump: 64-bit kod segmentine geçiş
    ; Bu satır işlemciyi resmen Long Mode'a sokar!
    push gdt64.code
    push .long_mode_start
    retf

[bits 64]
.long_mode_start:
    ; Data segment register'larını sıfırla (Long mode'da çoğu kullanılmaz ama temizlik iyidir)
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- 4. Kernel'a Giriş ---
    call kmain
    
    ; Eğer kernel dönerse işlemciyi durdur
    hlt

section .rodata
gdt64:
    dq 0 ; null descriptor
.code: equ $ - gdt64
    ; 64-bit kod segmenti: (1<<43): executable, (1<<44): descriptor type, 
    ; (1<<47): present, (1<<53): 64-bit flag (L bit)
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) 
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
