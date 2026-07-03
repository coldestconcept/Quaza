#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "pfs_types.h"
#include "pfs_builder.h"

int pfs_scan_directory(const char* path, pfs_inode_t* parent) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return -1;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) < 0) {
            perror("stat");
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // Create directory inode
            pfs_inode_t* dir_inode = pfs_inode_create(entry->d_name, 
                                                      PFS_INODE_TYPE_DIR, 
                                                      parent);
            if (dir_inode) {
                pfs_inode_add_child(parent, dir_inode);
                // Recursively scan subdirectory
                pfs_scan_directory(full_path, dir_inode);
            }
        } else if (S_ISREG(st.st_mode)) {
            // Create file inode
            pfs_inode_t* file_inode = pfs_inode_create(entry->d_name,
                                                       PFS_INODE_TYPE_FILE,
                                                       parent);
            if (file_inode) {
                file_inode->size = st.st_size;
                
                // Read file data
                if (st.st_size > 0) {
                    file_inode->data = malloc(st.st_size);
                    if (file_inode->data) {
                        FILE* fp = fopen(full_path, "rb");
                        if (fp) {
                            fread(file_inode->data, 1, st.st_size, fp);
                            fclose(fp);
                        }
                    }
                }
                
                pfs_inode_add_child(parent, file_inode);
            }
        }
    }
    
    closedir(dir);
    return 0;
}

void pfs_print_tree(pfs_inode_t* node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s%s (ino=%lu, size=%lu)\n", 
           node->type == PFS_INODE_TYPE_DIR ? "[D] " : "[F] ",
           node->name, node->ino, node->size);
    
    for (int i = 0; i < node->child_count; i++) {
        pfs_print_tree(node->children[i], depth + 1);
    }
}
