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
        __data_end__ = .;
    } > RAM AT > FLASH

    /* Uninitialized data (zeroed) */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM
}
