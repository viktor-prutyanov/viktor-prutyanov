bits 64
;SUB: pop 2 numbers, substarct and push
    pop rbx
    pop rax
    sub rax, rbx
    push rax