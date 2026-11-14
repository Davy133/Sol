; The user programs, built as ELF executables by the kernel Makefile.
section .rodata

global user_hello_start, user_hello_end
global user_fault_kernel_start, user_fault_kernel_end
global user_fault_code_start, user_fault_code_end

user_hello_start:
    incbin "arch/x86_64/user/hello.elf"
user_hello_end:

user_fault_kernel_start:
    incbin "arch/x86_64/user/fault_kernel.elf"
user_fault_kernel_end:

user_fault_code_start:
    incbin "arch/x86_64/user/fault_code.elf"
user_fault_code_end:

global user_shell_start, user_shell_end

user_shell_start:
    incbin "arch/x86_64/user/shell.elf"
user_shell_end:
