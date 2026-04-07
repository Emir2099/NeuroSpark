#ifndef EXT2FS_H
#define EXT2FS_H

#include "vfs.h"

typedef struct {
	uint32_t inode;
	uint32_t size;
	uint32_t mtime;
	char name[64];
} Ext2LsEntry;

const VfsBackendOps *ext2_backend_from_spec(const char *spec);
int ext2_list_root_long(Ext2LsEntry *out, int max_entries);

#endif
