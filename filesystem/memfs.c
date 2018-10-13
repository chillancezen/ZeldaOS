/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/memfs.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>
#include <filesystem/include/vfs.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <filesystem/include/fs_hierarchy.h>

static struct file_operation tmpfs_file_operation;
/*
 * XXX: Even file is created, the inner memory block is not guaranteed to be
 * allocated, the per-file operation must take care.
 */
static struct file * 
tmpfs_create_file(struct file_system * fs,
    const uint8_t * path,
    int mode)
{
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;
    int32_t result;
    struct file * file = NULL;
    struct file * root_file = (struct file *)fs->priv;
    ASSERT(root_file);
    ASSERT(root_file->type == FILE_TYPE_MARK);

    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    if (splitted_length > 1) {
        result = hierarchy_create_directory(&root_file->fs_node,
            splitted_path,
            splitted_length - 1);
        if (result != OK)
            return NULL;
    }
    result = hierarchy_create_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    if (result != OK)
        return NULL;
    file = hierarchy_search_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    ASSERT(file);
    file->priv = malloc(sizeof(struct mem_block_hdr));
    if (file->priv) {
        memset(file->priv, 0x0, sizeof(struct mem_block_hdr));
    }
    file->ops = &tmpfs_file_operation;
    return file;
}
static struct file *
tmpfs_open_file(struct file_system * fs, const uint8_t * path)
{
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;
    struct file * file = NULL;
    struct file * root_file = (struct file *)fs->priv;
    ASSERT(root_file);
    ASSERT(root_file->type == FILE_TYPE_MARK);
    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    file = hierarchy_search_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    return file;
}
static int32_t
tmpfs_create_dir(struct file_system * fs,
    const uint8_t * path)
{
    int32_t result = OK;
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;
    struct file * file;
    struct file * root_file = (struct file *)fs->priv;
    ASSERT(root_file);
    ASSERT(root_file->type == FILE_TYPE_MARK);
    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    hierarchy_create_directory(&root_file->fs_node,
        splitted_path,
        splitted_length);
    file = hierarchy_search_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    result = file && file->type == FILE_TYPE_DIR ? OK : -ERR_GENERIC;
    return result;
}
static void
__delete_file_entry(struct file * file)
{
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (block_hdr) {
        mem_block_raw_reclaim(block_hdr);
        free(block_hdr);
    }
    free(file);
}

static int32_t 
tmpfs_delete_file(struct file_system * fs,
    const uint8_t * path)
{
    int32_t result = OK;
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;
    struct file * root_file = (struct file *)fs->priv;
    ASSERT(root_file);
    ASSERT(root_file->type == FILE_TYPE_MARK);
    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    result = hierarchy_delete_file(&root_file->fs_node,
        splitted_path,
        splitted_length,
        __delete_file_entry);
    return result;
}

struct filesystem_operation tmpfs_ops = {
    .fs_open = tmpfs_open_file,
    .fs_create = tmpfs_create_file,
    .fs_mkdir = tmpfs_create_dir,
    .fs_delete = tmpfs_delete_file,
};


static struct file tmpfs_home_root_file;
static struct file_system tmpfs_home = {
    .filesystem_type = MEM_FS,
    .fs_ops = &tmpfs_ops,
    .priv = &tmpfs_home_root_file,
};

static struct file tmpfs_tmp_root_file;
static struct file_system tmpfs_tmp = {
    .filesystem_type = MEM_FS,
    .fs_ops = &tmpfs_ops,
    .priv = &tmpfs_tmp_root_file,
};

void
memfs_init(void)
{
    memset(&tmpfs_home_root_file, 0x0, sizeof(tmpfs_home_root_file));
    memset(&tmpfs_tmp_root_file, 0x0, sizeof(tmpfs_tmp_root_file));
    tmpfs_home_root_file.type = FILE_TYPE_MARK;
    tmpfs_tmp_root_file.type = FILE_TYPE_MARK;
    ASSERT(!register_file_system((uint8_t *)"/home/", &tmpfs_home));
    ASSERT(!register_file_system((uint8_t *)"/tmp/", &tmpfs_tmp));
}
