/* Memory Map for RP2350 Non-Secure World
 *
 * This MUST compliment the Secure World memory map.
 * NO OVERLAP allowed with Secure regions.
 *
 * Secure Flash: 0x10000000 - 0x1003FFFF (256KB)
 * Secure RAM:   0x20000000 - 0x2001FFFF (128KB)
 *
 * Therefore:
 * Non-Secure Flash starts at 0x10040000
 * Non-Secure RAM starts at 0x20020000
 */

MEMORY
{
    /* Non-Secure Flash: Approx 2MB - 256KB = 1792KB (assuming 2MB total flash)
     * Adjust LENGTH based on actual attached Flash size.
     */
    FLASH(rx) : ORIGIN = 0x10040000, LENGTH = 2048k - 256k
    
    /* Non-Secure RAM: Approx 520KB - 128KB = 392KB (assuming 520KB total RAM on RP2350)
     * Check datasheet for exact RAM size (RP2350 has 520KB SRAM).
     */
    RAM(rwx) : ORIGIN = 0x20020000, LENGTH = 520k - 128k
}

/* Define symbols for the startup code */
__StackTop = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* The vector table */
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

    /* Initialized data */
    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > RAM AT > FLASH

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM
}
