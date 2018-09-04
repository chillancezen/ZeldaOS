/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/vfs.h>
#include <lib/include/string.h>
#include <lib/include/errorcode.h>
#include <kernel/include/printk.h>

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
search_mount_entry(uint8_t * path)
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
    strcpy(mount_entries[target_index].mount_point, c_name);
    mount_entries[target_index].fs = fs;
    mount_entries[target_index].valid = 1;
    return OK;
}

#if defined(INLINE_TEST)
static void
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
    test_vfs();
#endif
}

__attribute__((constructor)) void
vfs_pre_init(void)
{
    memset(mount_entries, 0x0, sizeof(mount_entries));
}
