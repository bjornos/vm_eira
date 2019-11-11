;; this is a comment
dimd 0x02;
first_label:
mov r1, 40;
mov r2, 2;
mov r4, 9;
sub r4, r2;
sub r4, 1;
mov r5, 5;
add r5, r1;
;; test nop
nop ;
mov @8192, r1;
;;mov @8200, 8;
;mov r9,   ff
jmp first_label
halt;
