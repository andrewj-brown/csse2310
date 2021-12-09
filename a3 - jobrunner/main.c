#include <csse2310a3.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#ifndef SETUP_HEADER
#define SETUP_HEADER
#include "setup.h"
#endif

#ifndef RUN_HEADER
#define RUN_HEADER
#include "run.h"
#endif

#ifndef FINAL_HEADER
#define FINAL_HEADER
#include "finalise.h"
#endif

#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

#ifndef MAIN_HEADER
#define MAIN_HEADER
#include "main.h"
#endif

int main(int argc, char **argv)
{
    int verboseMode = parse_cmd_args(argc, argv);
    Job *firstJobInList = parse_job_files(argc, argv, verboseMode);
 
    #ifdef DEBUG
    verboseMode = 1;
    #endif

    if (verboseMode) {
        print_job_table(firstJobInList);
    }

    run_jobs(firstJobInList);
    
    setup_signal_kill_all();
    check_all_jobs_timer(firstJobInList);
    
    cleanup(firstJobInList);

    #ifdef DEBUG
    fprintf(stderr, "pausing for checkup...");
    while (1) {
    ;
    }
    #endif

    return 0;
}

Job *skip_to_end(Job *startOfList)
{
    while (startOfList->nextJob) {
        startOfList = startOfList->nextJob;
    }
    return startOfList;
}

int count_fields(char **counted)
{
    int i = 0;
    while (counted[i]) {
        i++;
    }
    return i;
}

void free_job(Job *unneeded)
{
    free(unneeded->cmd);
    
    int i = 0;
    while (unneeded->cmdArgs[i]) {
        free(unneeded->cmdArgs[i]);
        i++;
    }
    free(unneeded->cmdArgs);
    
    free(unneeded->pipeFromName);
    free(unneeded->pipeToName);

    if (unneeded->pipeFromLoc != -1) {
        close(unneeded->pipeFromLoc);
    }
    if (unneeded->pipeToLoc != -1) {
        close(unneeded->pipeToLoc);
    }

    free(unneeded);
}

int ll_length(Job *firstJobInList)
{
    int count = 0;
    while (firstJobInList) {
        firstJobInList = firstJobInList->nextJob;
        count++;
    }

    return count;
}

