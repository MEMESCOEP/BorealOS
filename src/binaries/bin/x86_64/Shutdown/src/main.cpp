// Simple application that uses ACPI to shutdown using a syscall.
int main() {
    // The syscall for shutdown is 0x0, with the first argument being the ACPI sleep state to enter (0x5 for shutdown).
    asm volatile (
        "mov $0x5, %rdi\n" // ACPI sleep state
        "mov $0x0, %rax\n" // Syscall number for shutdown
        "syscall\n"
    );

    return 1; // if we got here, the syscall failed for some reason, so return an error code.
}