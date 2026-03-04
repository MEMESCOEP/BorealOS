.section .text
.intel_syntax noprefix
/* IDT handling for x86_64 architecture */
.global IRQHandler
.global ExceptionHandler

/* Save all general-purpose registers in reverse order (r15 … rax) */
.macro SAVE_REGS
    push    r15
    push    r14
    push    r13
    push    r12
    push    r11
    push    r10
    push    r9
    push    r8
    push    rbp
    push    rdi
    push    rsi
    push    rdx
    push    rcx
    push    rbx
    push    rax
.endm

/* Restore registers in the opposite order (rax … r15) */
.macro RESTORE_REGS
    pop     rax
    pop     rbx
    pop     rcx
    pop     rdx
    pop     rsi
    pop     rdi
    pop     rbp
    pop     r8
    pop     r9
    pop     r10
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15
.endm

/* IRQ stub (no error code) */
.macro mac_IRQStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector          /* first argument: vector number */
    xor     rsi, rsi              /* second argument: error code = 0 */
    mov     rdx, rsp              /* third argument: pointer to saved registers */
    /* Align stack to 16 bytes and reserve space for call's return address */
    mov     rbp, rsp              /* save original rsp after SAVE_REGS */
    and     rsp, -16              /* align to 16 bytes */
    sub     rsp, 8                /* now rsp ≡ 8 (mod 16) for the upcoming call */
    call    IRQHandler
    mov     rsp, rbp              /* restore original rsp */
    RESTORE_REGS
    iretq
.endm

/* Exception stub with CPU‑pushed error code */
.macro mac_ISRErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector
    mov     rsi, QWORD PTR [rsp + 15*8]   /* error code is 15 registers above */
    mov     rdx, rsp
    mov     rbp, rsp
    and     rsp, -16
    sub     rsp, 8
    call    ExceptionHandler
    mov     rsp, rbp
    RESTORE_REGS
    add     rsp, 8                /* discard the error code */
    iretq
.endm

/* Exception stub without error code */
.macro mac_ISRNoErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector
    xor     rsi, rsi              /* error code = 0 */
    mov     rdx, rsp
    mov     rbp, rsp
    and     rsp, -16
    sub     rsp, 8
    call    ExceptionHandler
    mov     rsp, rbp
    RESTORE_REGS
    iretq
.endm

/* Define ISR and IRQ stubs */
mac_ISRNoErrStub 0
mac_ISRNoErrStub 1
mac_ISRNoErrStub 2
mac_ISRNoErrStub 3
mac_ISRNoErrStub 4
mac_ISRNoErrStub 5
mac_ISRNoErrStub 6
mac_ISRNoErrStub 7
mac_ISRErrStub 8
mac_ISRNoErrStub 9
mac_ISRErrStub 10
mac_ISRErrStub 11
mac_ISRErrStub 12
mac_ISRErrStub 13
mac_ISRErrStub 14
mac_ISRNoErrStub 15
mac_ISRNoErrStub 16
mac_ISRErrStub 17
mac_ISRNoErrStub 18
mac_ISRNoErrStub 19
mac_ISRNoErrStub 20
mac_ISRNoErrStub 21
mac_ISRNoErrStub 22
mac_ISRNoErrStub 23
mac_ISRNoErrStub 24
mac_ISRNoErrStub 25
mac_ISRNoErrStub 26
mac_ISRNoErrStub 27
mac_ISRNoErrStub 28
mac_ISRNoErrStub 29
mac_ISRErrStub 30
mac_ISRNoErrStub 31
mac_IRQStub 32
mac_IRQStub 33
mac_IRQStub 34
mac_IRQStub 35
mac_IRQStub 36
mac_IRQStub 37
mac_IRQStub 38
mac_IRQStub 39
mac_IRQStub 40
mac_IRQStub 41
mac_IRQStub 42
mac_IRQStub 43
mac_IRQStub 44
mac_IRQStub 45
mac_IRQStub 46
mac_IRQStub 47
mac_IRQStub 48
mac_IRQStub 49
mac_IRQStub 50
mac_IRQStub 51
mac_IRQStub 52
mac_IRQStub 53
mac_IRQStub 54
mac_IRQStub 55
mac_IRQStub 56
mac_IRQStub 57
mac_IRQStub 58
mac_IRQStub 59
mac_IRQStub 60
mac_IRQStub 61
mac_IRQStub 62
mac_IRQStub 63
mac_IRQStub 64
mac_IRQStub 65
mac_IRQStub 66
mac_IRQStub 67
mac_IRQStub 68
mac_IRQStub 69
mac_IRQStub 70
mac_IRQStub 71
mac_IRQStub 72
mac_IRQStub 73
mac_IRQStub 74
mac_IRQStub 75
mac_IRQStub 76
mac_IRQStub 77
mac_IRQStub 78
mac_IRQStub 79
mac_IRQStub 80
mac_IRQStub 81
mac_IRQStub 82
mac_IRQStub 83
mac_IRQStub 84
mac_IRQStub 85
mac_IRQStub 86
mac_IRQStub 87
mac_IRQStub 88
mac_IRQStub 89
mac_IRQStub 90
mac_IRQStub 91
mac_IRQStub 92
mac_IRQStub 93
mac_IRQStub 94
mac_IRQStub 95
mac_IRQStub 96
mac_IRQStub 97
mac_IRQStub 98
mac_IRQStub 99
mac_IRQStub 100
mac_IRQStub 101
mac_IRQStub 102
mac_IRQStub 103
mac_IRQStub 104
mac_IRQStub 105
mac_IRQStub 106
mac_IRQStub 107
mac_IRQStub 108
mac_IRQStub 109
mac_IRQStub 110
mac_IRQStub 111
mac_IRQStub 112
mac_IRQStub 113
mac_IRQStub 114
mac_IRQStub 115
mac_IRQStub 116
mac_IRQStub 117
mac_IRQStub 118
mac_IRQStub 119
mac_IRQStub 120
mac_IRQStub 121
mac_IRQStub 122
mac_IRQStub 123
mac_IRQStub 124
mac_IRQStub 125
mac_IRQStub 126
mac_IRQStub 127
mac_IRQStub 128
mac_IRQStub 129
mac_IRQStub 130
mac_IRQStub 131
mac_IRQStub 132
mac_IRQStub 133
mac_IRQStub 134
mac_IRQStub 135
mac_IRQStub 136
mac_IRQStub 137
mac_IRQStub 138
mac_IRQStub 139
mac_IRQStub 140
mac_IRQStub 141
mac_IRQStub 142
mac_IRQStub 143
mac_IRQStub 144
mac_IRQStub 145
mac_IRQStub 146
mac_IRQStub 147
mac_IRQStub 148
mac_IRQStub 149
mac_IRQStub 150
mac_IRQStub 151
mac_IRQStub 152
mac_IRQStub 153
mac_IRQStub 154
mac_IRQStub 155
mac_IRQStub 156
mac_IRQStub 157
mac_IRQStub 158
mac_IRQStub 159
mac_IRQStub 160
mac_IRQStub 161
mac_IRQStub 162
mac_IRQStub 163
mac_IRQStub 164
mac_IRQStub 165
mac_IRQStub 166
mac_IRQStub 167
mac_IRQStub 168
mac_IRQStub 169
mac_IRQStub 170
mac_IRQStub 171
mac_IRQStub 172
mac_IRQStub 173
mac_IRQStub 174
mac_IRQStub 175
mac_IRQStub 176
mac_IRQStub 177
mac_IRQStub 178
mac_IRQStub 179
mac_IRQStub 180
mac_IRQStub 181
mac_IRQStub 182
mac_IRQStub 183
mac_IRQStub 184
mac_IRQStub 185
mac_IRQStub 186
mac_IRQStub 187
mac_IRQStub 188
mac_IRQStub 189
mac_IRQStub 190
mac_IRQStub 191
mac_IRQStub 192
mac_IRQStub 193
mac_IRQStub 194
mac_IRQStub 195
mac_IRQStub 196
mac_IRQStub 197
mac_IRQStub 198
mac_IRQStub 199
mac_IRQStub 200
mac_IRQStub 201
mac_IRQStub 202
mac_IRQStub 203
mac_IRQStub 204
mac_IRQStub 205
mac_IRQStub 206
mac_IRQStub 207
mac_IRQStub 208
mac_IRQStub 209
mac_IRQStub 210
mac_IRQStub 211
mac_IRQStub 212
mac_IRQStub 213
mac_IRQStub 214
mac_IRQStub 215
mac_IRQStub 216
mac_IRQStub 217
mac_IRQStub 218
mac_IRQStub 219
mac_IRQStub 220
mac_IRQStub 221
mac_IRQStub 222
mac_IRQStub 223
mac_IRQStub 224
mac_IRQStub 225
mac_IRQStub 226
mac_IRQStub 227
mac_IRQStub 228
mac_IRQStub 229
mac_IRQStub 230
mac_IRQStub 231
mac_IRQStub 232
mac_IRQStub 233
mac_IRQStub 234
mac_IRQStub 235
mac_IRQStub 236
mac_IRQStub 237
mac_IRQStub 238
mac_IRQStub 239
mac_IRQStub 240
mac_IRQStub 241
mac_IRQStub 242
mac_IRQStub 243
mac_IRQStub 244
mac_IRQStub 245
mac_IRQStub 246
mac_IRQStub 247
mac_IRQStub 248
mac_IRQStub 249
mac_IRQStub 250
mac_IRQStub 251
mac_IRQStub 252
mac_IRQStub 253
mac_IRQStub 254
mac_IRQStub 255

/* Now we need an ISR stub table to reference these stubs */
.section .rodata
.align 8
.global ISRStubTable
ISRStubTable:
    .quad    stub_ISRStub_0
    .quad    stub_ISRStub_1
    .quad    stub_ISRStub_2
    .quad    stub_ISRStub_3
    .quad    stub_ISRStub_4
    .quad    stub_ISRStub_5
    .quad    stub_ISRStub_6
    .quad    stub_ISRStub_7
    .quad    stub_ISRStub_8
    .quad    stub_ISRStub_9
    .quad    stub_ISRStub_10
    .quad    stub_ISRStub_11
    .quad    stub_ISRStub_12
    .quad    stub_ISRStub_13
    .quad    stub_ISRStub_14
    .quad    stub_ISRStub_15
    .quad    stub_ISRStub_16
    .quad    stub_ISRStub_17
    .quad    stub_ISRStub_18
    .quad    stub_ISRStub_19
    .quad    stub_ISRStub_20
    .quad    stub_ISRStub_21
    .quad    stub_ISRStub_22
    .quad    stub_ISRStub_23
    .quad    stub_ISRStub_24
    .quad    stub_ISRStub_25
    .quad    stub_ISRStub_26
    .quad    stub_ISRStub_27
    .quad    stub_ISRStub_28
    .quad    stub_ISRStub_29
    .quad    stub_ISRStub_30
    .quad    stub_ISRStub_31
    .quad    stub_ISRStub_32
    .quad    stub_ISRStub_33
    .quad    stub_ISRStub_34
    .quad    stub_ISRStub_35
    .quad    stub_ISRStub_36
    .quad    stub_ISRStub_37
    .quad    stub_ISRStub_38
    .quad    stub_ISRStub_39
    .quad    stub_ISRStub_40
    .quad    stub_ISRStub_41
    .quad    stub_ISRStub_42
    .quad    stub_ISRStub_43
    .quad    stub_ISRStub_44
    .quad    stub_ISRStub_45
    .quad    stub_ISRStub_46
    .quad    stub_ISRStub_47
    .quad    stub_ISRStub_48
    .quad    stub_ISRStub_49
    .quad    stub_ISRStub_50
    .quad    stub_ISRStub_51
    .quad    stub_ISRStub_52
    .quad    stub_ISRStub_53
    .quad    stub_ISRStub_54
    .quad    stub_ISRStub_55
    .quad    stub_ISRStub_56
    .quad    stub_ISRStub_57
    .quad    stub_ISRStub_58
    .quad    stub_ISRStub_59
    .quad    stub_ISRStub_60
    .quad    stub_ISRStub_61
    .quad    stub_ISRStub_62
    .quad    stub_ISRStub_63
    .quad    stub_ISRStub_64
    .quad    stub_ISRStub_65
    .quad    stub_ISRStub_66
    .quad    stub_ISRStub_67
    .quad    stub_ISRStub_68
    .quad    stub_ISRStub_69
    .quad    stub_ISRStub_70
    .quad    stub_ISRStub_71
    .quad    stub_ISRStub_72
    .quad    stub_ISRStub_73
    .quad    stub_ISRStub_74
    .quad    stub_ISRStub_75
    .quad    stub_ISRStub_76
    .quad    stub_ISRStub_77
    .quad    stub_ISRStub_78
    .quad    stub_ISRStub_79
    .quad    stub_ISRStub_80
    .quad    stub_ISRStub_81
    .quad    stub_ISRStub_82
    .quad    stub_ISRStub_83
    .quad    stub_ISRStub_84
    .quad    stub_ISRStub_85
    .quad    stub_ISRStub_86
    .quad    stub_ISRStub_87
    .quad    stub_ISRStub_88
    .quad    stub_ISRStub_89
    .quad    stub_ISRStub_90
    .quad    stub_ISRStub_91
    .quad    stub_ISRStub_92
    .quad    stub_ISRStub_93
    .quad    stub_ISRStub_94
    .quad    stub_ISRStub_95
    .quad    stub_ISRStub_96
    .quad    stub_ISRStub_97
    .quad    stub_ISRStub_98
    .quad    stub_ISRStub_99
    .quad    stub_ISRStub_100
    .quad    stub_ISRStub_101
    .quad    stub_ISRStub_102
    .quad    stub_ISRStub_103
    .quad    stub_ISRStub_104
    .quad    stub_ISRStub_105
    .quad    stub_ISRStub_106
    .quad    stub_ISRStub_107
    .quad    stub_ISRStub_108
    .quad    stub_ISRStub_109
    .quad    stub_ISRStub_110
    .quad    stub_ISRStub_111
    .quad    stub_ISRStub_112
    .quad    stub_ISRStub_113
    .quad    stub_ISRStub_114
    .quad    stub_ISRStub_115
    .quad    stub_ISRStub_116
    .quad    stub_ISRStub_117
    .quad    stub_ISRStub_118
    .quad    stub_ISRStub_119
    .quad    stub_ISRStub_120
    .quad    stub_ISRStub_121
    .quad    stub_ISRStub_122
    .quad    stub_ISRStub_123
    .quad    stub_ISRStub_124
    .quad    stub_ISRStub_125
    .quad    stub_ISRStub_126
    .quad    stub_ISRStub_127
    .quad    stub_ISRStub_128
    .quad    stub_ISRStub_129
    .quad    stub_ISRStub_130
    .quad    stub_ISRStub_131
    .quad    stub_ISRStub_132
    .quad    stub_ISRStub_133
    .quad    stub_ISRStub_134
    .quad    stub_ISRStub_135
    .quad    stub_ISRStub_136
    .quad    stub_ISRStub_137
    .quad    stub_ISRStub_138
    .quad    stub_ISRStub_139
    .quad    stub_ISRStub_140
    .quad    stub_ISRStub_141
    .quad    stub_ISRStub_142
    .quad    stub_ISRStub_143
    .quad    stub_ISRStub_144
    .quad    stub_ISRStub_145
    .quad    stub_ISRStub_146
    .quad    stub_ISRStub_147
    .quad    stub_ISRStub_148
    .quad    stub_ISRStub_149
    .quad    stub_ISRStub_150
    .quad    stub_ISRStub_151
    .quad    stub_ISRStub_152
    .quad    stub_ISRStub_153
    .quad    stub_ISRStub_154
    .quad    stub_ISRStub_155
    .quad    stub_ISRStub_156
    .quad    stub_ISRStub_157
    .quad    stub_ISRStub_158
    .quad    stub_ISRStub_159
    .quad    stub_ISRStub_160
    .quad    stub_ISRStub_161
    .quad    stub_ISRStub_162
    .quad    stub_ISRStub_163
    .quad    stub_ISRStub_164
    .quad    stub_ISRStub_165
    .quad    stub_ISRStub_166
    .quad    stub_ISRStub_167
    .quad    stub_ISRStub_168
    .quad    stub_ISRStub_169
    .quad    stub_ISRStub_170
    .quad    stub_ISRStub_171
    .quad    stub_ISRStub_172
    .quad    stub_ISRStub_173
    .quad    stub_ISRStub_174
    .quad    stub_ISRStub_175
    .quad    stub_ISRStub_176
    .quad    stub_ISRStub_177
    .quad    stub_ISRStub_178
    .quad    stub_ISRStub_179
    .quad    stub_ISRStub_180
    .quad    stub_ISRStub_181
    .quad    stub_ISRStub_182
    .quad    stub_ISRStub_183
    .quad    stub_ISRStub_184
    .quad    stub_ISRStub_185
    .quad    stub_ISRStub_186
    .quad    stub_ISRStub_187
    .quad    stub_ISRStub_188
    .quad    stub_ISRStub_189
    .quad    stub_ISRStub_190
    .quad    stub_ISRStub_191
    .quad    stub_ISRStub_192
    .quad    stub_ISRStub_193
    .quad    stub_ISRStub_194
    .quad    stub_ISRStub_195
    .quad    stub_ISRStub_196
    .quad    stub_ISRStub_197
    .quad    stub_ISRStub_198
    .quad    stub_ISRStub_199
    .quad    stub_ISRStub_200
    .quad    stub_ISRStub_201
    .quad    stub_ISRStub_202
    .quad    stub_ISRStub_203
    .quad    stub_ISRStub_204
    .quad    stub_ISRStub_205
    .quad    stub_ISRStub_206
    .quad    stub_ISRStub_207
    .quad    stub_ISRStub_208
    .quad    stub_ISRStub_209
    .quad    stub_ISRStub_210
    .quad    stub_ISRStub_211
    .quad    stub_ISRStub_212
    .quad    stub_ISRStub_213
    .quad    stub_ISRStub_214
    .quad    stub_ISRStub_215
    .quad    stub_ISRStub_216
    .quad    stub_ISRStub_217
    .quad    stub_ISRStub_218
    .quad    stub_ISRStub_219
    .quad    stub_ISRStub_220
    .quad    stub_ISRStub_221
    .quad    stub_ISRStub_222
    .quad    stub_ISRStub_223
    .quad    stub_ISRStub_224
    .quad    stub_ISRStub_225
    .quad    stub_ISRStub_226
    .quad    stub_ISRStub_227
    .quad    stub_ISRStub_228
    .quad    stub_ISRStub_229
    .quad    stub_ISRStub_230
    .quad    stub_ISRStub_231
    .quad    stub_ISRStub_232
    .quad    stub_ISRStub_233
    .quad    stub_ISRStub_234
    .quad    stub_ISRStub_235
    .quad    stub_ISRStub_236
    .quad    stub_ISRStub_237
    .quad    stub_ISRStub_238
    .quad    stub_ISRStub_239
    .quad    stub_ISRStub_240
    .quad    stub_ISRStub_241
    .quad    stub_ISRStub_242
    .quad    stub_ISRStub_243
    .quad    stub_ISRStub_244
    .quad    stub_ISRStub_245
    .quad    stub_ISRStub_246
    .quad    stub_ISRStub_247
    .quad    stub_ISRStub_248
    .quad    stub_ISRStub_249
    .quad    stub_ISRStub_250
    .quad    stub_ISRStub_251
    .quad    stub_ISRStub_252
    .quad    stub_ISRStub_253
    .quad    stub_ISRStub_254
    .quad    stub_ISRStub_255

/* End of IDT.S */