/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <filesystem/include/vfs.h>
#include <lib/include/string.h>
#include <lib/include/errorcode.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>

static struct mount_entry mount_entries[MOUNT_ENTRY_SIZE];

void
dump_mount_entries(void)
{
    int idx = 0;
    int iptr = 0;
    LOG_INFO("Dump Virtual Filesystem mount entries:\n");
    for(idx = 0; idx < MOUNT_ENTRY_SIZE; idx++) {
        if(!mount_entries[idx].valid)
            continue;
        LOG_INFO("   %d. mount point: %s filesystem type: %s\n", iptr++,
            mount_entries[idx].mount_point,
            filesystem_type_to_name(mount_entries[idx].fs->filesystem_type));
    }
}
/*
 * split the path, store the splited string into `array` and
 * the total number of string into `iptr`
 * XXX: note the path must be canonicalized.
 */
void
split_path(uint8_t * path,
    uint8_t **array,
    int32_t * iptr)
{
    int span = 0;
    uint8_t * ptr = path;
    uint8_t * ptr_end = NULL;
    *iptr = 0;
    for (; *ptr; ) {
        for (ptr_end = ptr; *ptr_end && *ptr_end != '/'; ptr_end++);
        span = ptr_end - ptr;
        if (span == 0) {
            ASSERT(*ptr_end == '/');
            ptr += 1;
        } else {
            array[*iptr] = ptr;
            *iptr += 1;
            if (!*ptr_end)
                break;
            *ptr_end = '\x0';
            ptr = ptr_end + 1;
        }

    }
    {
        // Dump the splitted_path
        int idx = 0;
        LOG_TRIVIA("splitted path:[\n");
        for (idx = 0; idx < *iptr; idx++) {
            LOG_TRIVIA("    %s\n", array[idx]);
        }
        LOG_TRIVIA("]\n");
    }
}

/*
 * Given a path name, transform it to a canonical one
 * see in test_vfs for the cases and output
 */
int
canonicalize_path_name(uint8_t * dst, const uint8_t * src)
{
    int idx = 0;
    int idx1 = 0;
    int16_t iptr = 0;
    int16_t index_ptr = 0;
    int16_t word_start = -1;
    int16_t word_end = 0;
    int16_t word_len = 0;
    int16_t index_stack[256];
    memset(index_stack, 0x0, sizeof(index_stack));
    for (idx = 0; idx < MAX_PATH; idx++) {
        // No blank is allowed to appear in the path
        if (src[idx] == ' ')
            return -ERR_INVALID_ARG;
        if (src[idx] != '/' && src[idx])
            continue;
        if (word_start == -1 && src[idx]) {
            word_start = idx;
            continue;
        }
        if (word_start == -1 && !src[idx])
            break;
        /*At least '/' appears before.*/
        word_len = idx - word_start;
        if (word_len == 1) {
            /*
             * Give special care to '/'
             */
            if(!src[idx] && !index_ptr) {
                index_stack[index_ptr++] = word_start;
            } else {
                word_start = idx;
            }
        }else if(word_len == 2 && src[idx - 1] == '.') {
            word_start = idx;
        } else if (word_len == 3 &&
            src[idx - 1] == '.' &&
            src[idx - 2] == '.') {
            word_start = idx;
            index_ptr = index_ptr ? index_ptr -1 : 0;
        } else {
            index_stack[index_ptr++] = word_start;
            word_start = idx;
        }
        if (!src[idx])
            break;
    }
    /* Reform the path*/
    for(idx = 0; idx < index_ptr; idx++) {
        word_start = index_stack[idx];
        word_end = 0;
        for(idx1 = word_start + 1; idx1 < MAX_PATH; idx1++) {
            if(src[idx1] == '/') {
                word_end = idx1;
                break;   
            } else if (!src[idx1]) {
                word_end = idx1 + 1;
                break;
            }
        }
        for(idx1 = word_start; idx1 < word_end; idx1++) {
            dst[iptr++] = src[idx1];
        }
    }
    dst[iptr] = '\x0';
    return OK;
}
/*
 * Search the mount entry with LPM-like best fit policy
 * the root mount entry '/' is returned
 */
struct mount_entry *
search_mount_entry(const uint8_t * path)
{
    int idx = 0;
    int best_fit_index = -1;
    int best_fit_degree = 0;
    int idx_tmp = 0;
    int degree_tmp = 0;
    struct mount_entry * entry = NULL;
    uint8_t c_name[MAX_PATH];
    struct mount_entry * _entry = NULL;
    memset(c_name, 0x0, sizeof(c_name));
    ASSERT(!canonicalize_path_name(c_name, path));
    for(idx = 0; idx < MOUNT_ENTRY_SIZE; idx++) {
        _entry = &mount_entries[idx];
        if (!_entry->valid)
            continue;
        if (!start_with(c_name, _entry->mount_point)) {
            for(idx_tmp = 0; idx_tmp < MAX_PATH; idx_tmp++)
                if(!_entry->mount_point[idx_tmp]) {
                    degree_tmp = idx_tmp;
                    break;
                }
            if (best_fit_index == -1 || best_fit_degree < degree_tmp) {
                best_fit_index = idx;
                best_fit_degree = degree_tmp;
            }
        }
    }
    if (best_fit_index >= 0) {
        entry =  &mount_entries[best_fit_index];
    }
    return entry;
}

/*
 * Register a file system to the mount point.
 * non-zero returned indicates failure
 */
int
register_file_system(uint8_t * mount_point, struct file_system * fs)
{
    int idx = 0;
    int target_index = -1;
    int iptr = 0;
    uint8_t c_name[MAX_PATH];
    memset(c_name, 0x0, sizeof(c_name));
    ASSERT(!canonicalize_path_name(c_name, mount_point));
    /*
     * Check whether mount prefix conflicts.
     */
    for(idx = 0; idx < MOUNT_ENTRY_SIZE; idx++) {
        if(!mount_entries[idx].valid) {
            if (target_index == -1)
                target_index = idx;
            continue;
        }
        if (!start_with(mount_entries[idx].mount_point, c_name)) {
            /*
             * Even c_name is the the prefix of mount_entries[idx].mount_point
             * , there is extra check to exclude exception like:
             * mount_point: /home/jiezheng
             * c_name     : /home/jie
             */
            for(iptr = 0; c_name[iptr]; iptr++);
            if(!mount_entries[idx].mount_point[iptr] ||
                mount_entries[idx].mount_point[iptr] == '/') {
                LOG_ERROR("Path %s(conanical path:%s) conflicts with existing"
                    " mount point:%s\n",
                    mount_point,
                    c_name,
                    mount_entries[idx].mount_point);
                return -ERR_INVALID_ARG;
            }
        }
    }
    if(target_index < 0) {
        LOG_ERROR("the mount entries are running out.\n");
        return -ERR_OUT_OF_RESOURCE;
    }
    ASSERT(target_index < MOUNT_ENTRY_SIZE);
    memset(&mount_entries[target_index], 0x0, sizeof(struct mount_entry));
    strcpy_safe(mount_entries[target_index].mount_point, c_name,
        sizeof(mount_entries[target_index].mount_point));
    mount_entries[target_index].fs = fs;
    mount_entries[target_index].valid = 1;
    LOG_INFO("Registered file system, mount point:%s, type:%s\n",
        mount_entries[target_index].mount_point,
        filesystem_type_to_name(fs->filesystem_type));
    return OK;
}

#if defined(INLINE_TEST)
__attribute__((unused)) static void
test_vfs(void)
{
    uint8_t c_name[MAX_PATH];
    uint8_t * str0 = (uint8_t *)"/cute//foo/./bar/..";
    canonicalize_path_name(c_name, str0);
    LOG_DEBUG("test_vfs: %s canonical name is %s\n", str0, c_name);
    str0 =  (uint8_t *)"/..//.././foo/../bar/";
    canonicalize_path_name(c_name, str0);
    LOG_DEBUG("test_vfs: %s canonical name is %s\n", str0, c_name);
    str0 = (uint8_t *)"/dev/////.../mem/////";
    canonicalize_path_name(c_name, str0);
    LOG_DEBUG("test_vfs: %s canonical name is %s\n", str0, c_name);
    {
        struct mount_entry * entry;
        struct file_system fs0;
        fs0.filesystem_type = ZELDA_FS;
        ASSERT(!register_file_system((uint8_t *)"/", &fs0));
        ASSERT(!register_file_system((uint8_t *)"/home/jiezheng", &fs0));
        ASSERT(!register_file_system((uint8_t *)"/home/jie", &fs0));

        entry = search_mount_entry((uint8_t *)"//home/jied/../jie/./cute.txt");
        LOG_DEBUG("Found mount entry 0x%x(%s)\n", entry,
            entry ? (char *)entry->mount_point : "");
    }
    dump_mount_entries();
}
#endif
void
vfs_init(void)
{
#if defined(INLINE_TEST)
    //test_vfs();
#endif
}

__attribute__((constructor)) void
vfs_pre_init(void)
{
    memset(mount_entries, 0x0, sizeof(mount_entries));
}

/*
 * the VFS layer raw interface to open a file.
 * XXX: the `flags` and `mode` is unused
 */
struct file *
do_vfs_open(const uint8_t * path, uint32_t flags, uint32_t mode)
{
    uint8_t c_name[MAX_PATH];
    uint8_t sub_path[MAX_PATH];
    struct mount_entry * mount_entry = NULL;
    struct file * file = NULL;
    mount_entry = search_mount_entry(path);
    if (!mount_entry) {
        LOG_TRIVIA("can not find mount entry for path:%s\n", path);
        return NULL;
    }
    /*
     * Select the per-mount-entry and search the sub-path
     */
    {
        int iptr = 0;
        int iptr_dst = 0;
        memset(c_name, 0x0, sizeof(c_name));
        memset(sub_path, 0x0, sizeof(sub_path));
        canonicalize_path_name(c_name, path);
        for (iptr = 0; iptr < MAX_PATH; iptr++) {
            if (c_name[iptr] != mount_entry->mount_point[iptr])
                break;
        }
        //ASSERT(!mount_entry->mount_point[iptr]);
        for (; iptr < MAX_PATH && c_name[iptr]; iptr++) {
            sub_path[iptr_dst++] = c_name[iptr];
        }
    }
    /*
     * Invoke mount-entry-specific filesystem level OPEN interface
     */
    ASSERT(mount_entry->fs->fs_ops->fs_open);
    file = mount_entry->fs->fs_ops->fs_open(mount_entry->fs, sub_path);
    if (!file) {
        LOG_TRIVIA("Failed to open file:%s\n", c_name);
    } else {
        LOG_TRIVIA("vfs open file:0x%x(%s)\n",
            file, file->name);
        file->refer_count++;
    }
    return file;
}

/*
 * the VFS layer raw interface to close a file
 */
int32_t
do_vfs_close(struct file * file)
{
    file->refer_count--;
    LOG_TRIVIA("vfs close file:0x%x(%s)\n",
        file, file->name);
    return OK;
}

/*
 * the VFS layer raw interface to read file
 * the return value is categorized into three:
 * 1). negative, error occurs.
 * 2). positive, the number of byte read.
 * 3). ZERO, End of File.
 */
int32_t
do_vfs_read(struct file_entry * entry,
    void * buffer,
    int size)
{
    int32_t result = 0;
    ASSERT(entry->file->ops);
    ASSERT(entry->file->ops->read);
    result = entry->file->ops->read(
        entry->file,
        entry->offset,
        buffer,
        size);
    if (result > 0) {
        entry->offset += result;
    }
    return result;
}

int32_t
do_vfs_write(struct file_entry * entry,
    void * buffer,
    int size)
{
    int32_t result = 0;
    ASSERT(entry->file->ops);
    ASSERT(entry->file->ops->write);
    if (!entry->writable) {
        return -ERR_NOT_SUPPORTED;
    }
    result = entry->file->ops->write(
        entry->file,
        entry->offset,
        buffer,
        size);
    if (result > 0) {
        entry->offset += result;
    }
    return result;
}

int32_t
do_vfs_lseek(struct file_entry * entry, uint32_t offset, uint32_t whence)
{
    // Note that No sanity check is enforced here, 
    // Only calculate the new position
    switch (whence)
    {
        case SEEK_SET:
            entry->offset = offset;
            break;
        case SEEK_CUR:
            entry->offset += offset;
            break;
        case SEEK_END:
            ASSERT(entry->file->ops);
            ASSERT(entry->file->ops->size);
            entry->offset = (uint32_t)entry->file->ops->size(entry->file) +
                offset;
            break;
    }
    return (int32_t)entry->offset;
}
/*
 * To return OK once successful,
 * any other negative value indicates some error.
 *
 */
int32_t
do_vfs_truncate(struct file_entry * entry, uint32_t offset)
{
    ASSERT(entry->file->ops);
    if (!entry->file->ops->truncate) {
        return -ERR_NOT_SUPPORTED;
    }
    return entry->file->ops->truncate(entry->file, offset);
}

/*
 * Create a file in filesystem specific hierarchy.
 * return the `struct file *` if successful
 */
struct file *
do_vfs_create(const uint8_t * path, uint32_t flags, uint32_t mode)
{
    struct file * file = NULL;
    uint8_t c_name[MAX_PATH];
    uint8_t sub_path[MAX_PATH];
    struct mount_entry * mount_entry = NULL;
    mount_entry = search_mount_entry(path);
    if (!mount_entry) {
        LOG_TRIVIA("can not find mount entry for path:%s\n", path);
        return NULL;
    }
    // select the per-mount entry and search the sub-path
    {
        int iptr = 0;
        int iptr_dst = 0;
        memset(c_name, 0x0, sizeof(c_name));
        memset(sub_path, 0x0, sizeof(sub_path));
        canonicalize_path_name(c_name, path);
        for (iptr = 0; iptr < MAX_PATH; iptr++) {
            if (c_name[iptr] != mount_entry->mount_point[iptr])
                break;
        }
        ASSERT(!mount_entry->mount_point[iptr]);
        for (; iptr < MAX_PATH && c_name[iptr]; iptr++) {
            sub_path[iptr_dst++] = c_name[iptr];
        }
    }
    // try to invoke filesystem specific CREATE interface
    if (mount_entry->fs->fs_ops->fs_create) {
        file = mount_entry->fs->fs_ops->fs_create(mount_entry->fs,
            sub_path, mode);
    }
    if (file) {
        file->mode = mode;   
    }
    LOG_TRIVIA("create file:%s as 0x%x\n", c_name, file);
    return file;
}

/*
 * the VFS layer interface to create a directory entry 
 */
struct file *
do_vfs_directory_create(const uint8_t * path)
{
    uint8_t c_name[MAX_PATH];
    uint8_t sub_path[MAX_PATH];
    struct mount_entry  * mount_entry = NULL;
    struct file * file = NULL;
    mount_entry = search_mount_entry(path);
    if (!mount_entry) {
        LOG_TRIVIA("can not find mount entry for path:%s\n", path);
        return NULL;
    }
    {
        int iptr = 0;
        int iptr_dst = 0;
        memset(c_name, 0x0, sizeof(c_name));
        memset(sub_path, 0x0, sizeof(sub_path));
        canonicalize_path_name(c_name, path);
        for (iptr = 0; iptr < MAX_PATH; iptr++) {
            if (c_name[iptr] != mount_entry->mount_point[iptr])
                break;
        }
        ASSERT(!mount_entry->mount_point[iptr]);
        for (; iptr < MAX_PATH && c_name[iptr]; iptr++) {
            sub_path[iptr_dst++] = c_name[iptr];
        }
    }
    if (mount_entry->fs->fs_ops->fs_mkdir) {
        file = mount_entry->fs->fs_ops->fs_mkdir(mount_entry->fs, sub_path);
    }
    LOG_TRIVIA("create directory:%s as 0x%x\n", c_name, file);
    return file;
}
/*
 * teh VFS layer interface to delete a file/directory and all the sub files and
 * directories if it's not the leaf node
 * FIXME: currently it only check wether the file first file is opened, if it's 
 * directory entry, and sub-files are opened, things may get wrong, to fix it:
 * recursivly inspect sub-files and directorys.
 */
int32_t
do_vfs_path_delete(const uint8_t * path)
{
    int32_t result = OK;
    uint8_t c_name[MAX_PATH];
    uint8_t sub_path[MAX_PATH];
    struct file * file = NULL;
    struct mount_entry * mount_entry = search_mount_entry(path);
    if (!mount_entry) {
        LOG_TRIVIA("can not find mount entry for path:%s\n", path);
        return -ERR_INVALID_ARG;
    }
    {
        int iptr = 0;
        int iptr_dst = 0;
        memset(c_name, 0x0, sizeof(c_name));
        memset(sub_path, 0x0, sizeof(sub_path));
        canonicalize_path_name(c_name, path);
        for (iptr = 0; iptr < MAX_PATH; iptr++) {
            if (c_name[iptr] != mount_entry->mount_point[iptr])
                break;
        }
        ASSERT(!mount_entry->mount_point[iptr]);
        for (; iptr < MAX_PATH && c_name[iptr]; iptr++) {
            sub_path[iptr_dst++] = c_name[iptr];
        }
    }
    ASSERT(mount_entry->fs->fs_ops->fs_open);
    file = mount_entry->fs->fs_ops->fs_open(mount_entry->fs, sub_path);
    if (!file) {
        LOG_TRIVIA("Failed to find file:%s\n", c_name);
        return -ERR_NOT_FOUND;
    }
    if (file->refer_count) {
        LOG_TRIVIA("File:%s is in use\n", c_name);
        return -ERR_IN_USE;
    }
    if (!mount_entry->fs->fs_ops->fs_delete) {
        LOG_TRIVIA("FILE DELETE OPERATION not supported\n");
        return -ERR_NOT_SUPPORTED;
    }
    result = mount_entry->fs->fs_ops->fs_delete(mount_entry->fs, sub_path);
    return result;
}

/*
 * the VFS layer interface to fetch the state of the file
 */
int32_t
do_vfs_stat(const uint8_t * path, struct stat * buf)
{
    int32_t result = -ERR_GENERIC;
    uint8_t c_name[MAX_PATH];
    uint8_t sub_path[MAX_PATH];
    struct file * file = NULL;
    struct mount_entry * mount_entry = search_mount_entry(path);
    if (!mount_entry) {
        LOG_TRIVIA("can not find mount entry for path:%s\n", path);
        return -ERR_INVALID_ARG;
    }
    {
        int iptr = 0;
        int iptr_dst = 0;
        memset(c_name, 0x0, sizeof(c_name));
        memset(sub_path, 0x0, sizeof(sub_path));
        canonicalize_path_name(c_name, path);
        for (iptr = 0; iptr < MAX_PATH; iptr++) {
            if (c_name[iptr] != mount_entry->mount_point[iptr])
                break;
        }
        ASSERT(!mount_entry->mount_point[iptr]);
        for (; iptr < MAX_PATH && c_name[iptr]; iptr++) {
            sub_path[iptr_dst++] = c_name[iptr];
        }
    }
    ASSERT(mount_entry->fs->fs_ops->fs_open);
    file = mount_entry->fs->fs_ops->fs_open(mount_entry->fs, sub_path);
    if (!file) {
        LOG_TRIVIA("Failed to find file:%s\n", c_name);
        return -ERR_NOT_FOUND;
    }
    if (!file->ops->stat) {
        LOG_TRIVIA("stat not supported for file:%s\n", c_name);
        return -ERR_NOT_SUPPORTED;
    }
    memset(buf, 0x0, sizeof(struct stat));
    result = file->ops->stat(file, buf);
    return result;
}

