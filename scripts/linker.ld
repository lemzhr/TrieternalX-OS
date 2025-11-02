/* File: scripts/linker.ld */
ENTRY(_start) /* Titik masuk dari boot.asm */

SECTIONS
{
    /* Mulai di 1MB (0x100000) */
    . = 1M;

    /* Bagian .text (kode) */
    .text :
    {
        *(.multiboot) /* Header multiboot HARUS di awal */
        *(.text)
    }

    /* Bagian .rodata (data read-only) */
    .rodata :
    {
        *(.rodata)
    }

    /* Bagian .data (data yang diinisialisasi) */
    .data :
    {
        *(.data)
    }

    /* Bagian .bss (data yang tidak diinisialisasi, spt stack) */
    .bss :
    {
        *(.bss)
    }

    /* Membuang bagian yang tidak perlu */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}