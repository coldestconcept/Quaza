/* PS5 payload linker script.
 * Single RWE first segment (text + GOT together) — matches the layout used
 * by working payloads (game-compressor, kura-loader).  The GOT must be in
 * the RWE segment so that rtld_load() can write resolved symbol addresses
 * with plain volatile pointer writes (no kernel r/w needed).
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS {
        ph_text   PT_LOAD FLAGS (0x7);   /* RWE — text + GOT in one writable block */
        ph_data   PT_LOAD FLAGS (0x6);   /* RW  — regular data / bss               */
        ph_dyn    PT_DYNAMIC;
}

SECTIONS {
        /* ── RWE segment: code + relocation tables + GOT ───────────────── */
        .text : ALIGN(CONSTANT(MAXPAGESIZE)) {
            PROVIDE_HIDDEN (__text_start = .);
            *(.text .text.*)
            PROVIDE_HIDDEN (__text_end = .);
        } : ph_text

        .rodata : {
            *(.rodata .rodata.*)

            PROVIDE_HIDDEN(__init_array_start = .);
            KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
            KEEP(*(.init_array .ctors))
            PROVIDE_HIDDEN(__init_array_end = .);

            PROVIDE_HIDDEN(__fini_array_start = .);
            KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
            KEEP(*(.fini_array .dtors))
            PROVIDE_HIDDEN(__fini_array_end = .);
        } : ph_text

        /* RELA entries stay in the RWE segment — rtld reads them, no write needed */
        .rela : { *(.rela *.rela.*) } : ph_text

        /* GOT must be writable for rtld_load() symbol patching.
         * Keep it in ph_text (RWE) — same as game-compressor / kura-loader. */
        .got     : { *(.got .got.*)         } : ph_text
        .got.plt : { *(.got.plt .got.plt.*) } : ph_text

        /* ── RW segment: mutable data + BSS ────────────────────────────── */
        .data : ALIGN(CONSTANT(MAXPAGESIZE)) {
            *(.data .data.*)
        } : ph_data

        .dynamic : ALIGN(CONSTANT(MAXPAGESIZE)) {
            PROVIDE_HIDDEN (_DYNAMIC = .);
            *(.dynamic)
        } : ph_data : ph_dyn

        .bss (NOLOAD) : ALIGN(CONSTANT(MAXPAGESIZE)) {
            PROVIDE_HIDDEN (__bss_start = .);
            *(.bss .bss.*);
            *(COMMON)
            PROVIDE_HIDDEN (__bss_end = .);
        } : ph_data

        /DISCARD/ : {
            *(.note .note.*)
            *(.comment)
            *(.gnu.attributes)
            *(.llvm_jump_table_sizes)
            *(.relro_padding)
        }
}
