SECTION .text
global _start


;计算字符串长度
slen:
    push    ebx
    mov     ebx, eax

nextchar:
    cmp     byte[eax], 0
    jz      finished
    inc     eax
    jmp     nextchar

finished:
    sub     eax, ebx 
    pop     ebx
    ret


;打印字符串
sprint:
    push    edx
    push    ecx
    push    ebx
    push    eax
    call    slen

    mov     edx, eax
    pop     eax         ;eax保存要打印字符串的起始地址

    mov     ecx, eax
    mov     ebx, 1
    mov     eax, 4
    int     80h

    mov     eax, ecx
    pop     ebx
    pop     ecx
    pop     edx
    ret

sprintLF:
    call    sprint      ;打印字符串

    push    eax
    mov     eax, 0Ah    ;eax保存换行符
    push    eax
    mov     eax, esp    ;eax保存换行符的地址
    call    sprint
    pop     eax
    pop     eax
    ret

quit:
    mov     ebx, 0
    mov     eax, 1
    int     80h
    ret

iprint:
    push    eax
    push    ecx
    push    edx
    push    esi
    mov     ecx, 0

;统计要打印的整数的长度，并且加上每位加上48转为ascii格式后压栈
divideLoop:
    inc     ecx
    mov     edx, 0
    mov     esi, 10
    idiv    esi
    add     edx, 48
    push    edx
    cmp     eax, 0
    jnz     divideLoop

printLoop:
    dec     ecx
    mov     eax, esp    ;eax保存了要打印的数字的指针
    call    sprint
    pop     eax
    cmp     ecx, 0
    jnz     printLoop

    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret

iprintLF:
    call    iprint

    push    eax
    mov     eax, 0Ah
    push    eax
    mov     eax, esp
    call    sprint
    pop     eax
    pop     eax
    ret


Reverse:
    push    ebx
    push    ecx
    push    edx
    push    eax     ;eax指向计算结果的开头
    call    slen
    mov     ebx, eax    ;ebx存有长度
    dec     ebx         ;eax指向结果末尾
    pop     eax
    push    eax
    add     eax, ebx
    mov     ebx, eax
    pop     eax
    push    eax

    ;call    iprintLF
    ;mov     eax, ebx
    ;call    iprintLF

Loop:
    mov     cl, byte[eax]
    mov     dl, byte[ebx]
    mov     byte[eax], dl
    mov     byte[ebx], cl
    inc     eax
    dec     ebx
    cmp     eax, ebx
    jl      Loop


    pop     eax
    pop     edx
    pop     ecx
    pop     ebx
    ret



