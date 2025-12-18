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
    /* Non-Secure Flash: Approx 16MB - 256KB = 16128KB
     * Adjust LENGTH based on actual attached Flash size.
     */
    FLASH(rx) : ORIGIN = 0x10040000, LENGTH = 16384k - 256k
    
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
        . = ALIGN(4);
        __etext = .; /* Define __etext for crt0 */
    } > FLASH

    /* .init_array / .fini_array */
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

    /* Initialized data */
    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

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
    
    __StackLimit = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
    
    __end__ = .;
    end = __end__;
}

ASSERT(__StackTop >= __StackLimit, "Region RAM overflowed with stack")
