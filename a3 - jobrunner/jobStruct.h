/* i'm including libraries in one big block across all files just to make sure
 * everything has what it needs, no matter the compilation order.
 */
#ifndef LIBRARIES
#define LIBRARIES
#include <csse2310a3.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/**
 * A struct representing a job, with the command to be run, args for the
 * command, FILE * inputs and outputs, an integer timeout and a pointer
 * to the next job to be run (effectively creating a linked list).
 */
typedef struct Job {
    // number of this job
    int jobId;
    // command this job runs
    char *cmd;
    // null-terminated argument array for the job
    char **cmdArgs;
    // pipe "names" - i.e. the string provided in the jobfile
    char *pipeFromName;
    char *pipeToName;
    // pipe "locations" - i.e. pointer to file where the job reads/writes
    int pipeFromLoc;
    int pipeToLoc;
    // have a guess what this field represents
    int timeout;
    // pointer to another Job* struct, thus making a linked list
    struct Job *nextJob;
    // process ID of the fork'd child running this job
    pid_t jobPid;
} Job;

