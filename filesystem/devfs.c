/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/devfs.h>
#include <filesystem/include/fs_hierarchy.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>

static struct file devfs_root_file;
/*
 * This API is exported for other kernel components, do not give the interface
 * directly to user land application.
 */
struct file *
register_dev_node(struct file_system * fs,
    const uint8_t * path,
    uint32_t mode,
    struct file_operation * file_ops,
    void * file_priv)
{
    int32_t result;
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
    file->priv = file_priv;
    file->ops = file_ops;
    LOG_INFO("register dev node:%s with operation:0x%x\n",
        path, file_ops);
    return file;
}

static struct file *
devfs_open_file(struct file_system * fs, const uint8_t * path)
{
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;
    struct file * file = NULL;
    struct file * root_file = (struct file *)fs->priv;
    ASSERT(root_file);
    ASSERT(root_file->type == FILE_TYPE_MARK);
    if (!strcmp((uint8_t *)path, (uint8_t *)"/")) {
        return &devfs_root_file;
    }
    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    file = hierarchy_search_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    return file;
}
static struct filesystem_operation devfs_ops = {
    .fs_create = NULL,
    .fs_mkdir = NULL,
    .fs_delete = NULL,
    .fs_open = devfs_open_file,
};
static struct file_system dev_fs = {
    .filesystem_type = DEV_FS,
    .priv = &devfs_root_file,
    .fs_ops = &devfs_ops
};

struct file_system *
get_dev_filesystem(void)
{
    return &dev_fs;
}
void
devfs_init(void)
{
    ASSERT(!register_file_system((uint8_t *)"/dev", &dev_fs));
}

__attribute__((constructor)) static void
devfs_pre_init(void)
{
    memset(&devfs_root_file, 0x0, sizeof(devfs_root_file));
    devfs_root_file.type = FILE_TYPE_MARK;
}
