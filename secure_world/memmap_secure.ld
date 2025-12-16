/* Memory Map for RP2350 Secure World
 *
 * This is a SKELETON. The specific addresses depend on your security requirements.
 *
 * RP2350 Flash: 0x10000000 (XIP)
 * RP2350 SRAM:  0x20000000
 *
 * We split Flash and RAM into Secure (S) and Non-Secure (NS) regions.
 *
 * Example partition:
 * Flash: First 256KB for Secure, rest for Non-Secure
 * RAM:   First 128KB for Secure, rest for Non-Secure
 */

MEMORY
{
    /* Secure Flash: 256KB */
    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 256k
    
    /* Secure RAM: 128KB (SRAM0 + SRAM1 roughly, or just a linear chunk) */
    RAM(rwx) : ORIGIN = 0x20000000, LENGTH = 128k
    
    /* Non-Secure Callable (NSC) Region
     * This region MUST contain the SG (Secure Gateway) veneers.
     * It must be marked as Non-Secure Callable in the SAU (Security Attribution Unit).
     * Usually placed at the end of Secure Flash or a specific dedicated region.
     */
    NSC(rx) : ORIGIN = 0x10040000 - 4k, LENGTH = 4k /* Last 4KB of the Secure Flash allocation */
}

/* Define symbols for the startup code */
__StackTop = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* The vector table goes at the start of Flash */
    .vectors :
    {
        . = ALIGN(256);
        KEEP(*(.vectors))
    } > FLASH

    /* Code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        __etext = .; /* Define __etext for crt0 */
    } > FLASH

    .preinit_array : {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
    } > FLASH

    .init_array : {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > FLASH

    .fini_array : {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array))
        __fini_array_end = .;
    } > FLASH

    /* Non-Secure Callable Veneers 
     * The linker generates these veneers for functions decorated with `__attribute__((cmse_nonsecure_entry))`
     */
    .gnu.sgstubs :
    {
        . = ALIGN(32);
        *(.gnu.sgstubs*)
    } > NSC

    /* Initialized data (copied from Flash to RAM) */
    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    /* Uninitialized data (zeroed) */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
    
    /* Scratch regions often used by RP2040/RP2350 for fast execution.
       We map them to RAM for simplicity if we don't have separate banks. */
    .scratch_x : {
        __scratch_x_start__ = .;
        __scratch_x_source__ = .;
        __scratch_x_end__ = .;
    } > RAM
    
    .scratch_y : {
        __scratch_y_start__ = .;
        __scratch_y_source__ = .;
        __scratch_y_end__ = .;
    } > RAM
    
    /* End of stack check */
    __StackLimit = ORIGIN(RAM) + LENGTH(RAM) - 0x1000; /* Arbitrary check */
    
    /* Standard SDK symbols */
    __end__ = .;
    end = __end__;
}

/* Assertions */
ASSERT(__StackTop >= __StackLimit, "Region RAM overflowed with stack")

