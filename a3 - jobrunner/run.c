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

#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

#ifndef MAIN_HEADER
#define MAIN_HEADER
#include "main.h"
#endif

#ifndef RUN_HEADER
#define RUN_HEADER
#include "run.h"
#endif

void run_jobs(Job *firstJobInList)
{
    Job *jobIterator = firstJobInList;

    #ifdef DEBUG
    fprintf(stderr, "ppid:%i\n\n", getpid());
    #endif

    while (jobIterator) {
        jobIterator->jobPid = fork();
            
        if (jobIterator->jobPid == 0) {
            continue_as_child(firstJobInList, jobIterator);
            break;
        } else {
            // note that continue_as_parent covers moving to the next
            // jobIterator to continue the loop.
            jobIterator = continue_as_parent(jobIterator);
        }
    }
}

void clear_useless_information(Job *firstJob, Job *myJob)
{
    while (firstJob) {
        #ifdef DEBUG
        fprintf(stderr, "clearing job:%p:%i\n", (void*)firstJob, getpid());
        #endif
        
        if (firstJob == myJob) {
            firstJob = firstJob->nextJob;
            continue;
        } else {
            Job *nextJob = firstJob->nextJob;
            
            free_job(firstJob);
            
            firstJob = nextJob;
        }
    }

    return;
}

void continue_as_child(Job *firstJob, Job *myJob)
{
    clear_useless_information(firstJob, myJob);
    int numArgs = count_fields(myJob->cmdArgs);
    myJob->cmdArgs = realloc(myJob->cmdArgs, (numArgs + 2) * sizeof(char *));
 
    // go backwards over the loop and move every arg up one, making space
    // for the first arg to be the exec name
    for (int i = numArgs + 1; i >= 0; i--) {
        if (i != 0) {
            myJob->cmdArgs[i] = myJob->cmdArgs[i - 1];
        } else {
            myJob->cmdArgs[i] = myJob->cmd;
        }
    }
    
    #ifdef DEBUG
    fprintf(stderr, "Unsupressing child job (#%i) stderr\n\n", myJob->jobId);
    #endif
    #ifndef DEBUG
    int deadEnd = open(DEAD_END, O_RDONLY);
    dup2(deadEnd, STDERR_FD); // send all stderr to /dev/null, unless debug
    close(deadEnd);
    #endif
    
    if (myJob->pipeToLoc != -1) { 
        dup2(myJob->pipeToLoc, STDOUT_FD);
        if (myJob->pipeToLoc != STDOUT_FD) {
            close(myJob->pipeToLoc);
        }
    }
    
    if (myJob->pipeFromLoc != -1) {
        dup2(myJob->pipeFromLoc, STDIN_FD);
        if (myJob->pipeFromLoc != STDIN_FD) {
            close(myJob->pipeFromLoc);
        }
    }
    
    #ifdef DEBUG
    int i = 0;
    while (myJob->cmdArgs[i]) {
        fprintf(stderr, "%s\n", myJob->cmdArgs[i]);
        i++;
    }
    fprintf(stderr, "%s\n", myJob->cmdArgs[i]);
    #endif

    if (execvp(myJob->cmd, myJob->cmdArgs) < 0) {
        exit(BAD_FAILED_EXEC_ERRC);
    }
}

Job *continue_as_parent(Job *childJob)
{
    #ifdef DEBUG
    fprintf(stderr, "cpid:%i\n", childJob->jobPid);
    #endif

    if (childJob->pipeToLoc != -1) {
        close(childJob->pipeToLoc);
    }
    if (childJob->pipeFromLoc != -1) {
        close(childJob->pipeFromLoc);
    }

    return childJob->nextJob;
}

