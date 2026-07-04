/*
 * PS5 payload linker script
 * Produces a 4-segment layout matching pldmgr / game-compressor:
 *
 *   [0] PT_LOAD RWE  offset=0x4000 vaddr=0x0      ← .text
 *   [1] PT_LOAD RW   offset=*      vaddr=*         ← everything else except data/bss
 *   [2] PT_LOAD RW   offset=*      vaddr=*         ← .data + .bss
 *   [3] PT_DYNAMIC   offset=*      vaddr=*         ← .dynamic
 *
 * No PT_PHDR, no PT_GNU_STACK, no PT_GNU_RELRO — elfldr does not expect them.
 */

ENTRY(_start)

PHDRS {
    text  PT_LOAD    FLAGS(0x7);   /* RWE */
    data  PT_LOAD    FLAGS(0x6);   /* RW  */
    bss   PT_LOAD    FLAGS(0x6);   /* RW  */
    dyn   PT_DYNAMIC FLAGS(0x6);
}

SECTIONS {
    . = 0x0;

    /*
     * Text segment — starts at virtual address 0, file offset 0x4000.
     * The 16 KB gap (0x0000–0x3FFF) holds the ELF + program headers.
     */
    .text 0x0 : ALIGN(0x4000) {
        *(.text.start)
        *(.text .text.*)
    } :text

    . = ALIGN(0x4000);

    /* ── data segment: metadata + read-only + GOT + init/fini arrays ──── */
    .eh_frame_hdr : { *(.eh_frame_hdr) }                             :data
    .eh_frame     : { *(.eh_frame) }                                 :data
    .dynsym       : { *(.dynsym) }                                   :data
    .gnu.hash     : { *(.gnu.hash) }                                 :data
    .hash         : { *(.hash) }                                     :data
    .dynstr       : { *(.dynstr) }                                   :data
    .rela.dyn     : { *(.rela.dyn) }                                 :data
    .rela.plt     : { *(.rela.plt) }                                 :data
    .data.rel.ro  : { *(.data.rel.ro .data.rel.ro.*) }               :data
    .got          : { *(.got .got.plt) }                             :data
    .rodata       : { *(.rodata .rodata.*) }                         :data

    .init_array : ALIGN(8) {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(.init_array .init_array.*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } :data

    .fini_array : ALIGN(8) {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(.fini_array .fini_array.*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } :data

    .dynamic : ALIGN(0x4000) {
        *(.dynamic)
    } :data :dyn

    . = ALIGN(0x4000);

    /* ── bss segment: .data + .bss ──────────────────────────────────── */
    .data : ALIGN(0x4000) {
        *(.data .data.*)
    } :bss

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :bss

    /* Discard sections elfldr doesn't need */
    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
        *(.gnu.attributes)
        *(.llvm_jump_table_sizes)
        *(.relro_padding)
    }
}
