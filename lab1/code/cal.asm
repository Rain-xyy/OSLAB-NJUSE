%include    'lib.asm'

SECTION .data
msg1    db    'Please Input x and y :', 0h
x_sign  db      0, 0h
y_sign  db      0, 0h
add_sign:   db  0, 0h
mul_sign:   db  0, 0h

SECTION .bss
Input:  resb    80
x:      resb    40
y:      resb    40
add_result: resb    80
mul_result: resb    80
x_len:  resb    4
y_len:  resb    4
x_mul_pointer:  resb    4
y_mul_pointer:  resb    4
large_pointer:  resb    4       ;指向较大数字的高位
fact_sub_length:    resb    4   ;保存减法结果的长度
fack_sub_length:    resb    4




SECTION .text
global _start
;global main

;小端存储，高位数字保存在较低地址
_start:
;main:
    call    getInput

    mov     al, byte[x_sign]
    cmp     al, byte[y_sign]        
    jnz     callSub
    call    addWrapper          ;x、y同号，加法
    jmp     callMul

callSub:
    call    subWrapper

callMul:
    call    mulWarpper

    call    quit

subWrapper:
    push    eax
    push    ebx
    call    findGreater     ;将较大的数存在eax中，较小的数存在ebx中，结果符号保存在add_sign中
    mov     dword[large_pointer], eax   ;将较大数的地址存在large_pointer中，便于后续判断是否减完了

    call    sub

    mov     eax, add_result
    call    Reverse
    mov     eax, add_sign
    call    sprint
    mov     eax, add_result
    call    sprintLF
    pop     ebx
    pop     eax
    ret


findGreater:
    mov     eax, dword[x_len]   ;保存x长度
    mov     ebx, dword[y_len]   ;保存y长度
    push    ecx
    push    edx

    cmp     eax, ebx
    jg      XGreater            ;x位数更多，所以直接返回
    je      sameLength          ;x和y位数相同，特殊处理
    jl      YGreater            ;x位数少，x小，交换x和y


sameLength:
    mov     ecx, dword[x_len]   ;将剩余待比较的长度保存在ecx中
    dec     ecx
    mov     edx, -1              ;从高位开始比较，edx保存偏移的位数

cmpLoop:
    cmp     ecx, edx
    jz      equal

    inc     edx
    mov     al, byte[x + edx]
    cmp     al, byte[y + edx]
    jg      XGreater
    jl      YGreater
    je      cmpLoop

equal:
    mov     eax, x
    mov     ebx, y
    mov     byte[add_sign], 0
    mov     edi, x
    add     edi, dword[x_len]
    dec     edi
    mov     esi, y
    add     esi, dword[y_len]
    dec     esi
    jmp     Return3

XGreater:
    mov     al, byte[x_sign]
    mov     byte[add_sign], al
    mov     eax, x
    mov     ebx, y
    mov     edi, x
    add     edi, dword[x_len]
    dec     edi
    mov     esi, y
    add     esi, dword[y_len]
    dec     esi
    jmp     Return3

YGreater:
    mov     al, byte[y_sign]
    mov     byte[add_sign], al
    mov     eax, y
    mov     ebx, x
    mov     edi, y
    add     edi, dword[y_len]
    dec     edi
    mov     esi, x
    add     esi, dword[x_len]
    dec     esi
    jmp     Return3


Return3:
    pop     edx
    pop     ecx
    ret

;eax存被减数，ebx存减数;edi指向被减数最后一位，esi指向减数最后一位
sub:
    push    ecx     ;保存减法结果
    push    edx     ;保存进位
    mov     ecx, add_result
    mov     edx, 0

    jmp     subLoop

subLoop:
    inc     dword[fack_sub_length]  ;增加长度
    mov     al, byte[edi]
    mov     bl, byte[esi]

    sub     al, bl      ;本位相减
    sub     al, dl      ;减去借位
    mov     dl, 0       ;借位清零
    cmp     al, 0   
    jnl     enough
    add     al, 10      ;不够减，借位
    mov     dl, 1       ;设借位为1

enough:
    cmp     al, 0       ;如果al等于0，那么可能就是结果的位数到此为止
    jz      noIncreaseLength    ;暂时不改变长度
    push    eax
    mov     eax, dword[fack_sub_length]
    mov     dword[fact_sub_length], eax     ;在减法过程中，如果发现某一位不为0，说明可以更新长度了
    pop     eax


noIncreaseLength:
    mov     byte[ecx], al
    cmp     edi, dword[large_pointer]       ;判断大的那个数有没有减完
    jz      finished3
    inc     ecx
    dec     edi
    dec     esi
    jmp     subLoop

finished3:
    mov     ecx, add_result 
    ;结果每位加上48
Loop3:
    mov     al, byte[ecx]
    add     al, 48
    mov     byte[ecx], al
    inc     ecx
    dec     dword[fact_sub_length]
    cmp     dword[fact_sub_length], 0
    jng     Return4
    jmp     Loop3

Return4:
    pop     edx
    pop     ecx
    ret
 
mulWarpper:
    cmp     byte[x], 0      ;x以0开头，即x为0
    jz      notNegative

    cmp     byte[y], 0
    jz      notNegative

    ;打印乘法结果的符号
    mov     al, byte[x_sign]
    xor     al, byte[y_sign]
    mov     byte[mul_sign], al
    mov     eax, mul_sign
    call    sprint

notNegative:
    call    mul
    mov     eax, mul_result
    call    sprintLF

    ret


addWrapper:
    mov     eax, x_sign
    call    sprint
    call    add
    mov     eax, add_result
    call    sprintLF
    ret



;x+y，结果存储在x中
add:
    push    eax         ;存本位
    push    ebx         ;存本位
    push    ecx         ;存结果指针
    push    edx         ;存进位
    push    edi         ;存指针
    push    esi         ;存指针


    mov     edi, 0
    add     edi, x
    add     edi, dword[x_len]       ;指向x要加的末位
    dec     edi
    mov     esi, 0
    add     esi, y
    add     esi, dword[y_len]       ;指向y要加的末位
    dec     esi
    mov     ecx, add_result

    mov     edx, 0
    mov     eax, 0
    mov     ebx, 0

    jmp    addLoop
Return1:
    mov     eax, add_result
    call    Reverse

    pop     esi
    pop     edi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
    ret

addLoop:
    mov     al, byte[edi]
    mov     bl, byte[esi]
    add     al, bl        ;本位相加
    add     al, dl        ;加上进位
    add     al, 48
    mov     dl, 0
    cmp     al, 57        
    jng     L1
    mov     dl, 1
    sub     al, 10
L1:
    mov     byte[ecx], al

    cmp     edi, x   ;x加完了，把y剩余的位数加到x上
    jz      addY

    cmp     esi, y
    jz      addX      ;y加完了，直接结束

    inc     ecx
    dec     edi
    dec     esi
    
    jmp     addLoop

addY:   
    cmp     esi, y
    jz      finished1

    dec     esi
    inc     ecx
    mov     al, byte[esi]
    add     al, dl
    add     al, 48
    mov     dl, 0
    cmp     al, 57
    jng     L2
    mov     dl, 1
    sub     al, 10
L2:
    mov     byte[ecx], al
    jmp     addY

;要处理进位
addX:
    ;push    eax
    ;mov     eax, 100
    ;call    iprintLF
    ;pop     eax

    cmp     edi, x
    jz      finished1

    dec     edi
    inc     ecx
    mov     al, byte[edi]
    add     al, dl
    add     al, 48
    mov     dl, 0
    cmp     al, 57
    jng     L3
    mov     dl, 1
    sub     al, 10
L3:
    mov     byte[ecx], al
    jmp     addX
    
finished1:
    inc     ecx
    cmp     dl, 0
    jz      Return1
    add     dl, 48
    mov     byte[ecx], dl
    jmp     Return1




;获取输入的X和Y，连着负号读入（如果有的话)
getInput:
    mov     eax, msg1
    call    sprintLF

    mov     edx, 60         ;num of maxium bytes to read
    mov     ecx, Input      ;ecx loads the pointer pointing to x
    mov     ebx, 0          ;当有负号的时候，ebx设为-1，帮助写入X，因为有负号要跳过输入一位，但是不跳过X的一位
    mov     eax, 3
    int     80h

    call    setXAndY
    ret

setXAndY:
    push    edx
    push    esi
    push    eax
    push    ebx
    mov     esi, 0      
    mov     eax, 0      ;esi存储在字符串中的偏移量

findSpace:
    ;单独处理第一位（判断是否为负号）
    cmp     byte[Input + eax], '-'
    jnz     positive            ;非负，继续处理，不打扰了
    mov     byte[x_sign], '-'
    inc     eax
    inc     esi
    mov     ebx, -1

positive:
    cmp     byte[Input + eax], 32
    jz      getY
    mov     dl, byte[Input + eax]  
    sub     dl, 48  
    mov     byte[x + eax + ebx], dl     ;加当前字节到x中 
    inc     dword[x_len]  
    inc     eax
    mov     esi, eax
    inc     esi
    jmp     positive
getY:
    ;单独处理第一位（判断是否为负号）
    cmp     byte[Input + eax + 1], '-'
    jnz     positive1            ;非负，继续处理，不打扰了
    mov     byte[y_sign], '-'
    inc     eax                  ;Y为负, 将eax指向负号
    inc     esi

positive1:
    inc     eax     ;eax刚刚是指向空格，加1后指向第二个操作数的第一位(如果Y为负，eax跳过负号）
    cmp     byte[Input + eax], 10
    jz      Return
    mov     dl, byte[Input + eax]
    sub     dl, 48
    sub     eax, esi
    mov     byte[y + eax], dl
    inc     dword[y_len]
    add     eax, esi
    jmp     positive1
Return:
    pop     ebx
    pop     eax
    pop     esi
    pop     edx
    ret

mul:
    push    eax     ;存X当前位
    push    ebx     ;存Y当前位
    push    ecx     
    push    edx     
    push    edi     ;保存X当前位的指针,初始指向X末尾
    push    esi     ;保存Y当前位的指针，初始指向Y末尾

    mov     edi, x
    add     edi, dword[x_len]
    dec     edi
    mov     esi, y
    add     esi, dword[y_len]
    dec     esi
    mov     dword[x_mul_pointer], 0     ;表示X的偏移量
    mov     dword[y_mul_pointer], 0     ;表示Y的偏移量
    mov     ebx, 0
    mov     ecx, 0
    mov     edx, 0
    mov     dl, 10

    jmp    mulLoop

Return2:
    pop     esi
    pop     edi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
    ret

mulLoop:
L4:
    mov     dl, 10
    mov     ax, 0
    mov     al, byte[edi]      ;X当前位
    mov     bl, byte[esi]      ;Y当前位
    mul     bl                 ;乘法，结果保存在AX中
    add     ax, cx             ;加上进位
    div     dl                 ;AH存本位，AL存进位

    ;保存本位
    mov     edx, dword[x_mul_pointer]
    add     edx, dword[y_mul_pointer]
    add     edx, mul_result     ;edx保存结果应该保存在的地址（逆序）
    add     ah, byte[edx]       ;与之前已有的相加,现在ah中保留着计算完的本位，由于加上了上一轮的，可能超过10
    cmp     ah, 10
    jl      save
    sub     ah, 10
    inc     al                  ;进位加1
save:       
    mov     byte[edx], ah
    movzx   cx, al              ;无符号扩展，将进位保存在cx中

    cmp     esi, y
    jz      L5
    dec     esi
    inc     dword[y_mul_pointer]
    jmp     L4

L5:
    mov     edx, dword[x_mul_pointer]
    add     edx, dword[y_mul_pointer]
    add     edx, mul_result
    inc     edx                 ;一轮加完了，要保存进位
    mov     byte[edx], al
    mov     cx, 0               ;下一轮初始进位为0

    cmp     edi, x
    jz      finished2
    dec     edi
    inc     dword[x_mul_pointer]
    mov     dword[y_mul_pointer], 0
    mov     esi, y
    add     esi, dword[y_len]
    dec     esi
    jmp     L4


finished2:
    ;mov     eax, mul_result
    ;call    sprintLF

    inc     dword[x_mul_pointer]    ;处理最后一位进位
    mov     edx, dword[x_mul_pointer]
    add     edx, dword[y_mul_pointer]
    add     edx, mul_result
    mov     byte[edx], al

    mov     eax, mul_result
    jmp    add48

endLoop2:
    mov     eax, mul_result
    call    Reverse
    jmp     Return2

add48:
    mov     ebx, 0
    cmp     byte[x], 0
    jz      Loop2
    cmp     byte[y], 0
    jz      Loop2

    mov     ebx, dword[x_len]
    add     ebx, dword[y_len]       ;结果的位数最多为X和Y的位数之和,最少为X和Y的位数之和减一
    ;单独判断最后一轮有没有进位
    dec     ebx
    cmp     byte[mul_result + ebx], 0
    jnz     Loop2                   ;最后一位不为0，说明有进位
    dec     ebx
Loop2:
    mov     al, byte[mul_result + ebx]
    add     al, 48
    mov     byte[mul_result + ebx], al
    cmp     ebx, 0
    jz      endLoop2
    dec     ebx
    jmp     Loop2




    

