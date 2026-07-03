#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfs_types.h"

static uint64_t next_ino = 2; // 1 is reserved for root

pfs_inode_t* pfs_inode_create(const char* name, int type, pfs_inode_t* parent) {
    pfs_inode_t* inode = calloc(1, sizeof(pfs_inode_t));
    if (!inode) return NULL;
    
    inode->ino = next_ino++;
    inode->name = strdup(name);
    inode->type = type;
    inode->parent = parent;
    inode->children = NULL;
    inode->child_count = 0;
    inode->data = NULL;
    
    // Set mode based on type
    if (type == PFS_INODE_TYPE_DIR) {
        inode->mode = 0x41ED; // drwxr-xr-x
    } else {
        inode->mode = 0x81A4; // -rw-r--r--
    }
    
    return inode;
}

void pfs_inode_free(pfs_inode_t* inode) {
    if (!inode) return;
    
    if (inode->name) free(inode->name);
    if (inode->data) free(inode->data);
    
    // Free children recursively
    for (int i = 0; i < inode->child_count; i++) {
        pfs_inode_free(inode->children[i]);
    }
    free(inode->children);
    
    free(inode);
}

void pfs_inode_add_child(pfs_inode_t* parent, pfs_inode_t* child) {
    if (!parent || !child) return;
    
    parent->children = realloc(parent->children, 
                               (parent->child_count + 1) * sizeof(pfs_inode_t*));
    parent->children[parent->child_count++] = child;
    parent->nlink++;
}

pfs_inode_t* pfs_inode_find(pfs_inode_t* root, const char* path) {
    if (!root || !path) return NULL;
    
    if (strcmp(path, "/") == 0 || strlen(path) == 0) {
        return root;
    }
    
    // Simple path parsing (not handling .. etc)
    char* p = strdup(path);
    char* token = strtok(p, "/");
    
    pfs_inode_t* current = root;
    
    while (token) {
        int found = 0;
        for (int i = 0; i < current->child_count; i++) {
            if (strcmp(current->children[i]->name, token) == 0) {
                current = current->children[i];
                found = 1;
                break;
            }
        }
        if (!found) {
            free(p);
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    
    free(p);
    return current;
}
