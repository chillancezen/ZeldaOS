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

static int32_t
tmpfs_read_file(struct file * file, uint32_t offset, void * buffer, int size)
{
    int result = 0;
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (!block_hdr)
        return -ERR_GENERIC;
    result = mem_block_raw_read(block_hdr, offset, buffer, size);
    return result;
}

static int32_t
tmpfs_write_file(struct file * file, uint32_t offset, void * buffer, int size)
{
    int result = 0x0;
    uint32_t end_index = 0;
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (!block_hdr)
        return -ERR_GENERIC;
    if (block_hdr->nr_used == offset) {
        result = mem_block_raw_write_sequential(block_hdr, buffer, size);
    } else {
        result = mem_block_raw_write_random(block_hdr, offset, buffer, size);
    }
    if (result >= 0) {
        end_index = offset + result;
        block_hdr->nr_used = MAX(end_index, block_hdr->nr_used);
    }
    return result;
}

static int32_t
tmpfs_file_size(struct file * file)
{
    int32_t size = 0;
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (block_hdr) {
        size = block_hdr->nr_used;
    }
    return size;   
}

static int32_t
tmpfs_file_stat(struct file * file, struct stat * stat)
{
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    stat->st_mode = file->mode;
    stat->st_size = block_hdr->nr_used;
    return OK;
}

static int32_t
tmpfs_file_truncate(struct file * file, int offset)
{
    int32_t result = 0x0;
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (!block_hdr)
        return -ERR_GENERIC;
    result = mem_block_raw_truncate(block_hdr, offset);
    if (result != OK)
        return -ERR_GENERIC;
    block_hdr->nr_used = offset;
    return OK; 
}
static struct file_operation tmpfs_file_operation = {
    .read = tmpfs_read_file,
    .write = tmpfs_write_file,
    .size = tmpfs_file_size,
    .stat = tmpfs_file_stat,
    .truncate = tmpfs_file_truncate,
};

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
    if (!strcmp((uint8_t *)path, (uint8_t *)"/")) {
        return root_file;
    }
    memset(c_name, 0x0, sizeof(c_name));
    canonicalize_path_name(c_name, path);
    split_path(c_name, splitted_path, &splitted_length);
    file = hierarchy_search_file(&root_file->fs_node,
        splitted_path,
        splitted_length);
    return file;
}
static struct file *
tmpfs_create_dir(struct file_system * fs,
    const uint8_t * path)
{
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
    return file && file->type == FILE_TYPE_DIR ? file : NULL;
}
static void
__delete_file_entry(struct file * file)
{
    struct mem_block_hdr * block_hdr = (struct mem_block_hdr *)file->priv;
    if (block_hdr) {
        mem_block_raw_reclaim(block_hdr);
        // XXX: note the block_hdr is not allocated using get_mem_block,
        // here Just invoke free to reclaim it.
        free(block_hdr);
    }
    LOG_TRIVIA("tmpfs reclaiming file entry:0x%x(%s)\n", file,
        file->type == FILE_TYPE_MARK ? "FILE_TYPE_MARK" :
            file->type == FILE_TYPE_DIR ? "FILE_TYPE_DIR" :
            "FILE_TYPE_REGULAR");
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
    ASSERT(!register_file_system((uint8_t *)"/home/zelda/", &tmpfs_home));
    ASSERT(!register_file_system((uint8_t *)"/tmp/", &tmpfs_tmp));
#if !defined(INLINE_TEST)
    {
        int32_t result = OK;
        uint8_t * path = (uint8_t *)"//home/cute";
        struct file * file = do_vfs_create(path, O_RDWR, 0x777); 
        LOG_TRIVIA("vfs create:%x\n", file);
        ASSERT(file->type = FILE_TYPE_REGULAR);
        ASSERT(file == do_vfs_open(path, O_RDWR, 0x777));
        result = do_vfs_path_delete(path);
        ASSERT(result != OK);
        ASSERT(do_vfs_close(file) == OK);
        result = do_vfs_path_delete(path);
        ASSERT(result == OK);
        ASSERT(!do_vfs_open(path, O_RDWR, 0x777));
        ASSERT(file = do_vfs_directory_create(path));
        ASSERT(file == do_vfs_open(path, O_RDWR, 0x0));
        ASSERT(OK == do_vfs_close(file));
        ASSERT(file->type == FILE_TYPE_DIR);
        ASSERT(file->refer_count == 0);
        ASSERT(!do_vfs_create(path, O_RDWR, 0x777));
        path = (uint8_t *)"/home/cute//bar";
        ASSERT(do_vfs_create(path, O_RDWR, 0x777));
        file = do_vfs_open(path, O_RDWR, 0x777);
        ASSERT(file && file->type == FILE_TYPE_REGULAR);
        ASSERT(OK != do_vfs_path_delete(path));
        ASSERT(do_vfs_close(file) == OK);
        ASSERT(do_vfs_path_delete((uint8_t *)"/home/cute/") == OK);
        ASSERT(!do_vfs_open((uint8_t *)"/home/cute/", O_RDWR, 0x777));
        ASSERT(!do_vfs_open((uint8_t *)"/home/cute/bar", O_RDWR, 0x777));

        path = (uint8_t *)"/home/cute//bar/../avater";
        file = do_vfs_create(path, O_RDWR, 0x777);
        ASSERT(file && file->type == FILE_TYPE_REGULAR);
        ASSERT(file == do_vfs_open((uint8_t *)"/home/cute/../cute/./avater",
            O_RDWR, 0x0));
        ASSERT(!do_vfs_open((uint8_t *)"/home/cute/bar", O_RDWR, 0));
        {
            int result = 0x0;
            uint8_t buffer[256];
            struct file_entry entry;
            entry.file = file;
            entry.offset = 0x0;
            entry.writable = 0x1;
            result = do_vfs_read(&entry, buffer, 16);
            ASSERT(result == 0);
            result = do_vfs_write(&entry, "Hello World", 11);
            ASSERT(result == 11);
            result = do_vfs_read(&entry, buffer, 16);
            ASSERT(!result);
            do_vfs_lseek(&entry, 11, SEEK_SET);
            result = do_vfs_read(&entry, buffer, 10);
            ASSERT(result == 0);
            do_vfs_lseek(&entry, 0x0, SEEK_END);
            result = do_vfs_write(&entry, "FOO", 3);
            ASSERT(result == 3);
            do_vfs_lseek(&entry, 0, SEEK_SET);
            memset(buffer, 0x0, sizeof(buffer));
            result = do_vfs_read(&entry, buffer, 256);
            result = do_vfs_write(&entry, "", 4084);
        }
        ASSERT(!do_vfs_close(file));
        ASSERT(do_vfs_path_delete(path) == OK);
    }
#endif
}
