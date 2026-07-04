/*
 * Browser (WebAssembly) entry point for libprosperopkg.
 *
 * This is the SAME PKG-building code that runs on the PS5 payload
 * (pkg_builder.c / pfs_builder.c / npdrm.c) — it is portable C with no PS5-
 * specific syscalls, so it can be compiled with Emscripten and run directly
 * in a desktop or mobile browser. That lets a PC/Android user convert a
 * game dump to a PKG locally, without a jailbroken PS5 or its payload
 * running — the same code just reads from/writes to Emscripten's in-memory
 * filesystem (MEMFS) instead of the PS5's real disk.
 *
 * The JS side (www/js/wasm-builder.js) is responsible for:
 *   1. Writing the user's selected folder into MEMFS at a fixed path.
 *   2. Calling wasm_pkg_build() via Module.ccall().
 *   3. Reading the produced .pkg file back out of MEMFS with FS.readFile()
 *      and triggering a browser download.
 */
#include <stdio.h>
#include <string.h>
#include "pkg_types.h"
#include "pkg_header.h"

/*
 * Build a PKG from `input_dir` (an already-populated MEMFS directory) into
 * `output_path` (also inside MEMFS). Returns a pkg_result_t (0 on success).
 */
int wasm_pkg_build(const char *content_id, const char *input_dir,
                    const char *output_path) {
    if (!content_id || !input_dir || !output_path) {
        return PKG_ERROR_INVALID_PARAM;
    }

    pkg_project_t project;
    memset(&project, 0, sizeof(project));
    strncpy(project.content_id, content_id, sizeof(project.content_id) - 1);
    project.type = PKG_TYPE_PS5_GD;
    project.pfs_block_size = 65536; /* matches PFS_BLOCK_SIZE in pfs_types.h */
    project.use_encryption = false;
    project.is_patch = false;

    return pkg_build(&project, input_dir, output_path);
}
