/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdio.h>
#include <builtin.h>
#include <zelda.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#define MAX_TASKENT      (1024)
enum task_state {
    TASK_STATE_ZOMBIE = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_INTERRUPTIBLE,
    TASK_STATE_UNINTERRUPTIBLE,
    TASK_STATE_EXITING,
    TASK_STATE_MAX,
};
static char *
state_to_string(int state)
{
    char * ret = "unknown";
    switch(state)
    {
        case TASK_STATE_RUNNING:
            ret = "running";
            break;
        case TASK_STATE_INTERRUPTIBLE:
            ret = "interruptible";
            break;
        case TASK_STATE_UNINTERRUPTIBLE:
            ret = "uninterruptible";
            break;
        case TASK_STATE_EXITING:
            ret = "exiting";
            break;
        default:
            break;
    }
    return ret;
}

int
main(int argc, char * argv[])
{
    int idx = 0;
    struct taskent * taskp = malloc(MAX_TASKENT * sizeof(struct taskent));
    int32_t nr_task = 0;
    nr_task = gettaskents(taskp, MAX_TASKENT);
    for (idx = 0; idx < nr_task; idx++) {
        printf("%5d PL%d:0x%-8x %-25s %-25s %-25s\n",
            taskp[idx].task_id,
            taskp[idx].privilege_level,
            taskp[idx].entry,
            taskp[idx].name,
            state_to_string(taskp[idx].state),
            taskp[idx].cwd);
    }
    return 0;
}
