/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/zeldafs.h>
#include <memory/include/physical_memory.h>
#include <kernel/include/printk.h>
#include <lib/include/list.h>
#include <lib/include/string.h>
#include <kernel/include/elf.h>
#include <kernel/include/task.h>
#include <filesystem/include/vfs.h>
#include <memory/include/malloc.h>

static uint32_t zelda_drive_start = (uint32_t)&_zelda_drive_start;
static uint32_t zelda_drive_end =(uint32_t)&_zelda_drive_end;

static struct file zelda_root_node;


static struct list_elem head = {
    .prev = NULL,
    .next = NULL   
};

struct zelda_file *
search_zelda_file(char * name)
{
    struct zelda_file * _file;
    struct list_elem * _list;
    LIST_FOREACH_START(&head, _list) {
        _file = CONTAINER_OF(_list, struct zelda_file, list);
        if (!strcmp(_file->path, (uint8_t *)name))
            return _file;
    }
    LIST_FOREACH_END();
    return NULL;
}
/*
 * create the directory hierarchies. note multiple vertical directory entries
 * can be created at one time.
 */
static int32_t
zeldafs_create_directory(uint8_t ** splitted_path, int iptr)
{
    int idx = 0;
    struct generic_tree * _node = NULL;
    struct file * _file = NULL;
    int found = 0;
    struct generic_tree * current_node = &zelda_root_node.fs_node;
    for (idx = 0; idx < iptr; idx++) {
        // Try to find the directory entry with the same name
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        // Validate the found directory entry
        if (found) {
            if (_file->type != FILE_TYPE_DIR) {
                return -ERR_EXIST;
            }
            current_node = &_file->fs_node;
        } else {
            // The directory does not exist, Try to create one.
            _file = malloc(sizeof(struct file));
            if (!_file) {
                LOG_TRIVIA("Can not allocate file descriptor\n");
                return -ERR_OUT_OF_MEMORY;
            }
            memset(_file, 0x0, sizeof(struct file));
            strcpy(_file->name, splitted_path[idx]);
            _file->type = FILE_TYPE_DIR;
            _file->mode = 0x0;
            _file->priv = NULL;
            ASSERT(!add_sibling(current_node, &_file->fs_node));
            current_node = &_file->fs_node;
        }
        // Prepare for next lower hierarchy
        ASSERT(_file);
        if (!current_node->left) {
            // No child on current node
            struct file * _file_another = malloc(sizeof(struct file));
            if (!_file_another) {
                LOG_TRIVIA("Can not allocate another file descriptor\n");
                return -ERR_OUT_OF_MEMORY;
            }
            memset(_file_another, 0x0, sizeof(struct file));
            _file_another->type = FILE_TYPE_MARK;
            ASSERT(!add_child(current_node, &_file_another->fs_node));
        }
        current_node = current_node->left;
    }
    return OK;
}

/*
 * Create the file descriptor in the ZELDA FS hierarchies
 * XXX: the directory is not created automatically.
 * if the directory is not present, failure will be notified to CALLER.
 */
static int32_t
zeldafs_create_file(uint8_t ** splitted_path, int iptr)
{
    int idx = 0;
    int found = 0;
    struct file * _file = NULL;
    struct generic_tree * _node = NULL;
    struct generic_tree * current_node = &zelda_root_node.fs_node;
    if (iptr < 1) {
        return -ERR_INVALID_ARG;
    }
    for (idx = 0; idx < (iptr - 1); idx++) {
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        if (!found) {
            return -ERR_NOT_PRESENT;
        }
        current_node = _file->fs_node.left;
    }
    // Till this, we arrive at the layer of the target file node, we search the
    // target file first.
    found = 0;
    FOREACH_SIBLING_NODE_START(current_node, _node) {
        _file = CONTAINER_OF(_node, struct file, fs_node);
        if (!strcmp(_file->name, splitted_path[iptr - 1])) {
            found = 1;
            break;
        }
    }
    FOREACH_SIBLING_NODE_END();
    if (found) {
        return -ERR_EXIST;
    } else {
        // File not exist, try to create one
        _file = malloc(sizeof(struct file));
        memset(_file, 0x0, sizeof(struct file));
        strcpy(_file->name, splitted_path[iptr -1]);
        _file->type = FILE_TYPE_REGULAR;
        _file->mode = 0x0;
        _file->priv = NULL;
        ASSERT(!add_sibling(current_node, &_file->fs_node));
    }
    return OK;
}
/*
 * seach a file in zelda fs hierarchy.
 * return NULL if not found, otherwise, return `struct file *`
 */
struct file *
zeldafs_search_file(uint8_t ** splitted_path, int iptr)
{
    int idx = 0;
    int found = 0;
    struct file * result = NULL;
    struct file * _file = NULL;
    struct generic_tree * _node = NULL;
    struct generic_tree * current_node = &zelda_root_node.fs_node;
    for (idx = 0; idx < iptr; idx++) {
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        if (!found) {
            _file = NULL;
            break;
        }
        current_node = _file->fs_node.left;
    }
    result = _file;
    return result;
}

/*
 * This is to construct the filesystem directory tree from zelda drive
 */
void
construct_zelda_drive_hierarchy(void)
{
    struct zelda_file * _file;
    struct list_elem * _list;
    struct file * file;
    uint8_t c_name[MAX_PATH];
    uint8_t * splitted_path[MAX_PATH];
    int32_t splitted_length;

    LIST_FOREACH_START(&head, _list) {
        _file = CONTAINER_OF(_list, struct zelda_file, list);
        splitted_length = 0;
        canonicalize_path_name(c_name, _file->path);
        split_path(c_name, splitted_path, &splitted_length);
        zeldafs_create_directory(splitted_path, splitted_length -1);
        zeldafs_create_file(splitted_path, splitted_length);
        file = zeldafs_search_file(splitted_path, splitted_length);
        if (file) {
            file->priv = _file;
            LOG_TRIVIA("Succeeded to insert file:%s into zelda fs "
                "hierarchies\n", _file->path);
        } else {
            LOG_ERROR("Failed to insert file:%s into zelda fs hierarchies\n",
                _file->path);
        }
    }
    LIST_FOREACH_END();
}
/*
 * perform a breadth-fisrt traversal in zelda fs and dump the relationship.
 */
void
dump_zelda_filesystem(void)
{
    struct list_elem queue = {
        .prev = NULL,
        .next = NULL   
    };
    struct list_elem * _list = NULL;
    struct file * _file = NULL;
    struct file * _parent_file = NULL;
    struct generic_tree * _parent_node = NULL;
    struct generic_tree * _node = NULL;
    struct generic_tree * current_node = &zelda_root_node.fs_node;
    
    LOG_INFO("Dump Zelda FS hierarchies\n");
    list_init(&current_node->list);
    list_append(&queue, &current_node->list);

    while (!list_empty(&queue)) {
        _list = list_fetch(&queue);
        ASSERT(_list);
        _node = CONTAINER_OF(_list, struct generic_tree, list);
        _file = CONTAINER_OF(_node, struct file, fs_node);
        if (_node->left) {
            list_init(&_node->left->list);
            list_append(&queue, &_node->left->list);
        }
        if (_node->right) {
            list_init(&_node->right->list);
            list_append(&queue, &_node->right->list);
        }
        _parent_node = parent_of_node(_node);
        _parent_file = CONTAINER_OF(_parent_node, struct file, fs_node);

        switch (_file->type)
        {
            case FILE_TYPE_MARK:
                LOG_INFO("  FILE_TYPE_MARK(0x%x): parent:%s\n", _file,
                    _parent_node ? _parent_file->name: (uint8_t *)"");
                break;
            case FILE_TYPE_DIR:
                LOG_INFO("  FILE_TYPE_DIR(0x%x): %s parent:%s\n", _file,
                    _file->name,
                    _parent_node ? _parent_file->name: (uint8_t *)"");
                break;
            case FILE_TYPE_REGULAR:
                LOG_INFO("  FILE_TYPE_REGULAR(0x%x): %s parent:%s\n", _file,
                    _file->name,
                    _parent_node ? _parent_file->name: (uint8_t *)"");
                break;
            default:
                break;
        }
    }
}

void
dump_zedla_drives(void)
{
    struct zelda_file * _file;
    struct list_elem * _list;
    LOG_INFO("Dump Zelda Drive:\n");
    LIST_FOREACH_START(&head, _list) {
        _file = CONTAINER_OF(_list, struct zelda_file, list);
        LOG_INFO("   File name:%s size:%d\n", _file->path, _file->length);
    }
    LIST_FOREACH_END();
}

void
enumerate_files_in_zelda_drive(void)
{
    struct zelda_file * _file = NULL;
    uint32_t iptr = zelda_drive_start;
    for(; iptr < zelda_drive_end;) {
        _file = (struct zelda_file *)iptr; 
        _file->list.next = NULL;
        _file->list.prev = NULL;
        list_append(&head, &_file->list);
        iptr += sizeof(struct zelda_file) + _file->length;
    }
}

static struct file *
zelda_fs_open(struct file_system * fs, const uint8_t * path)
{
    uint8_t * splitted_path[MAX_PATH];
    int32_t iptr = 0;
    split_path((uint8_t *)path, splitted_path, &iptr);

    printk("sub path:%s\n", path);
    return NULL;
}

struct filesystem_operation zeldafs_ops = {
    .fs_open = zelda_fs_open   
};

struct file_system zeldafs = {
    .filesystem_type = ZELDA_FS,
    .fs_ops = &zeldafs_ops
};

void
zeldafs_init(void)
{
    memset(&zelda_root_node, 0x0, sizeof(struct file));
    zelda_root_node.type = FILE_TYPE_MARK;
    enumerate_files_in_zelda_drive();
    construct_zelda_drive_hierarchy();
    dump_zedla_drives();
    //dump_zelda_filesystem();
    ASSERT(!register_file_system((uint8_t *)"/", &zeldafs));
#if 0
    {
        int rc = 0, rc1;
        struct zelda_file * _file = search_zelda_file("/usr/bin/dummy");
        printk("found zelda file:%x\n", _file);
        rc = validate_static_elf32_format(_file->content, _file->length);
        rc1 = load_static_elf32(_file->content, (uint8_t *)"A=b  B=\"cute adorable\" /usr/bin/dummy 'foo' bar \"bar cute\" 34");
        printk("%d %d\n", rc, rc1);

        rc = validate_static_elf32_format(_file->content, _file->length);
        rc1 = load_static_elf32(_file->content, (uint8_t *)"A=b  B=\"cute adorable\" /usr/bin/dummy 212");
        if(0){
            struct list_elem * task_head = get_task_list_head();
            struct list_elem * _list;
            struct task * _task;
            struct task * tasks[256];
            int iptr = 0;
            int idx = 0;
            LIST_FOREACH_START(task_head, _list) {
                _task = CONTAINER_OF(_list, struct task, list);
                tasks[iptr++] = _task;
            }
            LIST_FOREACH_END();
            for(idx = 0; idx < iptr; idx++) {
                reclaim_task(tasks[idx]);
            }
        }
        dump_tasks();
    }
#endif
}
