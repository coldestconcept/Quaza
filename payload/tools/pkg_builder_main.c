/*
 * pkg_builder_main.c — native host wrapper around libprosperopkg's pkg_build().
 *
 * Compiled and run in Termux (Android ARM64) at build time to produce
 * quaza-tile.pkg from the sce_sys/ staging directory.  The resulting PKG
 * is then embedded in the ELF via gen_embed_pkg.py + tile_pkg_embed.h.
 *
 * Build:
 *   clang -O2 \
 *     payload/libprosperopkg/src/pkg_builder.c \
 *     payload/libprosperopkg/src/pfs_builder.c \
 *     payload/libprosperopkg/src/pfs_directory.c \
 *     payload/libprosperopkg/src/pfs_inode.c \
 *     payload/libprosperopkg/src/npdrm.c \
 *     payload/libprosperopkg/src/utils.c \
 *     payload/tools/pkg_builder_main.c \
 *     -I payload/libprosperopkg/include \
 *     -lm -o /tmp/quaza_pkgbuild
 *
 * Run:
 *   /tmp/quaza_pkgbuild payload/sce_sys /tmp/quaza-tile.pkg
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pkg_types.h"
#include "pkg_header.h"

int pkg_build(pkg_project_t *project, const char *input_dir,
              const char *output_path);

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <sce_sys_parent_dir> <output.pkg>\n",
                argv[0]);
        fprintf(stderr, "  e.g. %s payload/sce_sys /tmp/quaza-tile.pkg\n",
                argv[0]);
        return 1;
    }

    const char *input_dir = argv[1];
    const char *output    = argv[2];

    pkg_project_t proj;
    memset(&proj, 0, sizeof(proj));

    strncpy(proj.content_id, "EP0000-QUAZ00001_00-QUAZAPKGTOOL0001",
            sizeof(proj.content_id) - 1);

    /* build the sce_sys path from the supplied input_dir */
    char sce_sys[1024];
    snprintf(sce_sys, sizeof(sce_sys), "%s", input_dir);
    strncpy(proj.sce_sys_path, sce_sys, sizeof(proj.sce_sys_path) - 1);
    strncpy(proj.app_path,     input_dir, sizeof(proj.app_path) - 1);

    proj.type           = PKG_TYPE_PS5_GD;
    proj.pfs_block_size = 65536;
    proj.use_encryption = 0;
    proj.is_patch       = 0;

    printf("[host] Building tile PKG...\n");
    printf("[host]   input  : %s\n", input_dir);
    printf("[host]   output : %s\n", output);

    int rc = pkg_build(&proj, input_dir, output);
    if (rc != 0) {
        fprintf(stderr, "[host] pkg_build() failed: %d\n", rc);
        return rc;
    }

    printf("[host] Done — %s\n", output);
    return 0;
}
