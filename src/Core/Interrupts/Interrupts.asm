EXTERN ExceptionHandler
EXTERN IRQHandler
GLOBAL tbl_ISRStubTable

;%macro mac_IRQStub 1
;stub_ISRStub_%+%1:
;    push dword 0
;    push dword %1
;    call IRQHandler
;    add esp, 8
;    iret
;%endmacro

%macro mac_IRQStub 1
stub_ISRStub_%+%1:
    pusha
    ;push dword 0
    push dword %1
    call IRQHandler
    add esp, 4 ;8
    popa
    iret
%endmacro

%macro mac_ISRErrStub 1
stub_ISRStub_%+%1:
    push dword %1
    call ExceptionHandler
    add esp, 8
    iret
%endmacro

%macro mac_ISRNoErrStub 1
stub_ISRStub_%+%1:
    push dword %1
    call ExceptionHandler
    add esp, 8
    iret
%endmacro

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


tbl_ISRStubTable:
%assign i 0
%rep 48
    dd stub_ISRStub_%+i
    %assign i i+1
%endrep