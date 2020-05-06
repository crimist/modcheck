# modcheck

Demo for checking if stack is inside main module - detects calls from DLL injection

Easily defeated by hooking `RtlCaptureStackBackTrace` or any of the other module info funcs
