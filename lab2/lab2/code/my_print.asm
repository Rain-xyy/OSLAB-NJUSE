global sprint



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

    mov     eax, [esp + 4]      ;eax指向待打印的字符串

    push    eax
    call    slen
    mov     edx, eax        ;edx保存长度
    pop     eax
    mov     ecx, eax
    mov     ebx, 1          ;ecx保存要打印字符串的起始地址
    mov     eax, 4
    int     80h


    ret
