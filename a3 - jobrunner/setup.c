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

#ifndef SETUP_HEADER
#define SETUP_HEADER
#include "setup.h"
#endif

#ifndef MAIN_HEADER
#define MAIN_HEADER
#include "main.h"
#endif

#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

Job *limit_openable_files(Job *jobIterator)
{
    Job *prevJob = NULL;
    Job *firstRunnableJob = jobIterator;

    while (jobIterator) {
        int failed = 0;

        if (!good_filenames(jobIterator)) {
            if (prevJob) {
                prevJob->nextJob = prevJob->nextJob->nextJob;
            } else {
                firstRunnableJob = jobIterator->nextJob;
            }
            failed = 1;
        } else {
            // only assign prevJob if the job is actually valid
            prevJob = jobIterator;
        }

        if (failed) {
            Job *nextJob = jobIterator->nextJob;
            free_job(jobIterator);
            jobIterator = nextJob;
        } else {
            jobIterator = jobIterator->nextJob;
        }
    }

    return firstRunnableJob;
}

Job *limit_workable_pipes(Job *jobIterator, bool firstPass)
{
    Job *prevJob = NULL;
    Job *firstRunnableJob = jobIterator;

    // "large" null-terminated list (maximum number of pipes will be one per
    // end per job) because then we never have to realloc() which means the
    // pointer to the start of this list shouldn't change, so we don't have
    // to update our reference to it
    char **alreadyPrinted = malloc(
            (ll_length(jobIterator) + 1) * 2 * sizeof(char *));
    alreadyPrinted[0] = NULL;

    while (jobIterator) {
        int failed = 0;

        if (!good_pipes(jobIterator, firstRunnableJob,
                alreadyPrinted, firstPass)) {
            if (prevJob) {
                prevJob->nextJob = prevJob->nextJob->nextJob;
            } else {
                firstRunnableJob = jobIterator->nextJob;
            }
            
            failed = 1;
        } else {
            // only assign prevJob if the job is actually valid
            prevJob = jobIterator;
        }
        
        if (failed) {
            Job *nextJob = jobIterator->nextJob;            

            free_job(jobIterator);
            jobIterator = nextJob;
        } else {
            jobIterator = jobIterator->nextJob;
        }
    }

    int i = 0;
    while (alreadyPrinted[i]) {
        free(alreadyPrinted[i]);
        i++;
    }
    free(alreadyPrinted);

    return firstRunnableJob;
}

int good_filenames(Job *checkJob)
{
    int noFailures = 1;

    if (strcmp(checkJob->pipeFromName, DEFAULT_IO)) {
        if (checkJob->pipeFromName[0] != PIPE_IDENTIFIER) {
            // input name isn't "nothing" and isn't a "pipe"
            int newFd = open(checkJob->pipeFromName, O_RDONLY, PERMS_ALL);
            
            if (newFd < 0) {
                noFailures = 0;
                fprintf(stderr, BAD_READ_FILENAME, checkJob->pipeFromName);
            } else {
                checkJob->pipeFromLoc = newFd;
            }
        }
    }
    
    if (strcmp(checkJob->pipeToName, DEFAULT_IO)) {
        if (checkJob->pipeToName[0] != PIPE_IDENTIFIER) {
            int newFd = open(checkJob->pipeToName, O_WRONLY | O_CREAT,
                    PERMS_ALL);

            if (newFd < 0) {
                noFailures = 0;
                fprintf(stderr, BAD_WRITE_FILENAME, checkJob->pipeToName);
            } else {
                checkJob->pipeToLoc = newFd;
            }
        }
    }

    return noFailures;
}

Job *match_from_end(Job *jobIterator, Job *checkJob)
{
    Job *matchedEnd = NULL;

    while (jobIterator) {
        #ifdef DEBUG
        fprintf(stderr, "matchfromend from:%s:%i to:%s:%i\n",
                checkJob->pipeFromName, checkJob->jobId,
                jobIterator->pipeToName, checkJob->jobId);
        #endif

        if (!strcmp(checkJob->pipeFromName, jobIterator->pipeToName)) {
            if (matchedEnd == NULL) {
                matchedEnd = jobIterator;
            } else {
                matchedEnd = NULL;
                break;
            }
        }

        jobIterator = jobIterator->nextJob;
    }

    return matchedEnd;
}

Job *match_to_end(Job *jobIterator, Job *checkJob)
{
    Job *matchedEnd = NULL;
    
    while (jobIterator) {
        #ifdef DEBUG
        fprintf(stderr, "matchtoend from:%s:%p to:%s:%p\n",
                jobIterator->pipeFromName, (void *)jobIterator,
                checkJob->pipeToName, (void *)checkJob);
        #endif

        if (!strcmp(checkJob->pipeToName, jobIterator->pipeFromName)) { 
            if (matchedEnd == NULL) {
                matchedEnd = jobIterator;
            } else {
                matchedEnd = NULL;
                break;
            }
        }

        jobIterator = jobIterator->nextJob;
    }

    return matchedEnd;
}

int good_pipes(Job *checkJob, Job *firstRunnableJob, char **alreadyPrinted,
        bool firstPass)
{
    int noFailures = 1;
    Job *fromMatchedEnd = NULL;
    Job *jobIterator = firstRunnableJob;

    if (checkJob->pipeFromName[0] == PIPE_IDENTIFIER) {
        fromMatchedEnd = match_from_end(jobIterator, checkJob);

        if (!fromMatchedEnd || two_read_ends(jobIterator, checkJob)) {
            noFailures = 0;
            for (int i = 0; i < strlen(checkJob->pipeFromName); i++) {
                checkJob->pipeFromName[i] = checkJob->pipeFromName[i + 1];
            }

            print_pipe_once(checkJob->pipeFromName, alreadyPrinted, firstPass);
        }
    }
    jobIterator = firstRunnableJob;

    if (checkJob->pipeToName[0] == PIPE_IDENTIFIER) {
        Job *toMatchedEnd = match_to_end(jobIterator, checkJob);

        if (!toMatchedEnd || two_write_ends(jobIterator, checkJob)) {
            noFailures = 0;
            for (int i = 0; i < strlen(checkJob->pipeToName); i++) {
                checkJob->pipeToName[i] = checkJob->pipeToName[i + 1];
            }

            #ifdef DEBUG
            fprintf(stderr, "pipeToFail %s\n", checkJob->pipeToName);
            #endif

            print_pipe_once(checkJob->pipeToName, alreadyPrinted, firstPass);
        }
    }
    jobIterator = firstRunnableJob;

    if (noFailures && fromMatchedEnd) {
        open_pipes(checkJob, fromMatchedEnd);
    }

    return noFailures;
}

void print_pipe_once(char *pipeName, char **alreadyPrinted, bool firstPass)
{
    if (!firstPass) {
        return;
    }

    int i = 0;
    while (alreadyPrinted[i]) {
        // for every pipe we've printed, if it matches this pipe, skip it
        if (!strcmp(alreadyPrinted[i], pipeName)) {
            return;
        }

        i++;
    }

    // [i] is the number of the null-terminator of this list
    // replace the null-terminator with the new pipeName, then add a new
    // null-terminator
    alreadyPrinted[i] = malloc((strlen(pipeName) + 1) * sizeof(char));
    strcpy(alreadyPrinted[i], pipeName);
    alreadyPrinted[i + 1] = NULL;

    fprintf(stderr, BAD_PIPE_USAGE, pipeName);

    return;
}

int two_read_ends(Job *jobIterator, Job *checkJob)
{
    while (jobIterator) {
        if (jobIterator == checkJob) {
            jobIterator = jobIterator->nextJob;
            continue;
        }

        if (!strcmp(checkJob->pipeFromName, jobIterator->pipeFromName)) {
            return 1;
        } else {
            jobIterator = jobIterator->nextJob;
        }
    }

    return 0;
}

int two_write_ends(Job *jobIterator, Job *checkJob)
{
    while (jobIterator) {
        if (jobIterator == checkJob) {
            jobIterator = jobIterator->nextJob;
            continue;
        }

        if (!strcmp(checkJob->pipeToName, jobIterator->pipeToName)) {
            return 1;
        } else {
            jobIterator = jobIterator->nextJob;
        }
    }
    
    return 0;
}

void open_pipes(Job *fromJob, Job *toJob)
{
    int fd[2];
    pipe(fd);
    fromJob->pipeFromLoc = fd[0];
    toJob->pipeToLoc = fd[1];
    return;
}

void print_job_table(Job *printJob)
{
    while (printJob) {
        fprintf(stderr, "%i:%s:%s:%s:%i", printJob->jobId, printJob->cmd,
                printJob->pipeFromName, printJob->pipeToName, 
                printJob->timeout);

        int argCount = 0;
        while (printJob->cmdArgs[argCount]) {
            fprintf(stderr, ":%s", printJob->cmdArgs[argCount]);
            argCount++;
        }
        
        #ifdef DEBUG
        fprintf(stderr, " | %i:%i:%p:%i",
                printJob->pipeFromLoc, printJob->pipeToLoc,
                (void *)printJob->nextJob, printJob->jobPid);
        #endif
        
        fprintf(stderr, "\n");
        printJob = printJob->nextJob;
    }

    #ifdef DEBUG
    fprintf(stderr, "\n");
    #endif
    
    fflush(stderr);
    return;
}

Job *parse_job_files(int argc, char **argv, int verboseMode)
{
    Job *firstJobInList = NULL;

    // 1+verboseMode lets us start at the first file arg
    for (int i = 1 + verboseMode; i < argc; i++) {
        Job *nextListStart = parse_job_file(argv[i]);

        if (firstJobInList == NULL) {
            firstJobInList = nextListStart;
        } else {
            Job *lastJob = skip_to_end(firstJobInList);
            lastJob->nextJob = nextListStart;
        }
    }
    
    assign_job_ids(firstJobInList);

    firstJobInList = limit_openable_files(firstJobInList);

    // horrific kludge. keep running limit_workable_pipes until calling it
    // removes no pipes. 
    int lastLen = ll_length(firstJobInList);
    bool firstPass = true; // only print errors on the first pass
    while (1) {
        firstJobInList = limit_workable_pipes(firstJobInList, firstPass);
        firstPass = false;

        if (lastLen == ll_length(firstJobInList)) {
            break;
        } else {
            lastLen = ll_length(firstJobInList);
        }
    }

    if (!firstJobInList) {
        fprintf(stderr, BAD_NO_JOBS_ERRM);
        exit(BAD_NO_JOBS_ERRC);
    }

    return firstJobInList;
}

void assign_job_ids(Job *firstJobInList)
{
    int newJobNum = 1;

    while (firstJobInList) {
        firstJobInList->jobId = newJobNum;
        firstJobInList = firstJobInList->nextJob;
        newJobNum++;
    }

    return;
}

Job *parse_job_file(char *fileAddr)
{
    FILE *jobFile = open_job_file(fileAddr);
    char *line;
    int lineNum = 0;
    Job *firstJobInFile = NULL;

    while ((line = read_line(jobFile))) {
        lineNum++;
        if (line[0] == COMMENT_IDENTIFIER || empty_line(line)) {
            free(line);
            continue;
        }

        char **jobInfo = split_by_commas(line);
        int numFields = count_fields(jobInfo);

        if (!validate_line(jobInfo, numFields)) {
            fclose(jobFile);
            free(jobInfo);
            free(line);

            fprintf(stderr, BAD_JOBLINE_ERRM, lineNum, fileAddr);
            exit(BAD_JOBLINE_ERRC);
        }

        if (firstJobInFile == NULL) {
            firstJobInFile = create_job_from_line(jobInfo, numFields,
                    lineNum, fileAddr);
        } else {
            Job *lastJob = skip_to_end(firstJobInFile);
            lastJob->nextJob = create_job_from_line(jobInfo,
                    numFields, lineNum, fileAddr);
        }

        free(jobInfo);
        free(line);
    }

    free(line);
    fclose(jobFile);
    return firstJobInFile;
}

Job *create_job_from_line(char **jobInfo, int numFields, int lineNum,
        char *fileAddr)
{
    Job *jobBeingParsed = init_new_job();
    
    jobBeingParsed->cmd = realloc(jobBeingParsed->cmd, 
            (strlen(jobInfo[0]) + 1) * sizeof(char));
    strcpy(jobBeingParsed->cmd, jobInfo[0]);

    jobBeingParsed->pipeFromName = realloc(jobBeingParsed->pipeFromName,
            (strlen(jobInfo[1]) + 1) * sizeof(char));
    strcpy(jobBeingParsed->pipeFromName, jobInfo[1]);

    jobBeingParsed->pipeToName = realloc(jobBeingParsed->pipeToName,
            (strlen(jobInfo[2]) + 1) * sizeof(char));
    strcpy(jobBeingParsed->pipeToName, jobInfo[2]);

    if (numFields >= 4) {
        if (strlen(jobInfo[3]) > 0) {
            if (atoi(jobInfo[3]) == 0 && jobInfo[3][0] != '0') {
                fprintf(stderr, BAD_JOBLINE_ERRM, lineNum, fileAddr);
                exit(BAD_JOBLINE_ERRC);
            }
            jobBeingParsed->timeout = atoi(jobInfo[3]);
        } else {
            jobBeingParsed->timeout = 0;
        }

        if (numFields >= 5) {
            jobBeingParsed->cmdArgs = realloc(jobBeingParsed->cmdArgs,
                    (numFields - 3) * sizeof(char *));

            for (int i = 4; i < numFields; i++) {
                jobBeingParsed->cmdArgs[i - 4] = malloc(
                        (strlen(jobInfo[i]) + 1) * sizeof(char));
                strcpy(jobBeingParsed->cmdArgs[i - 4], jobInfo[i]);

                jobBeingParsed->cmdArgs[i - 3] = NULL;
            }
        }
    } else {
        jobBeingParsed->timeout = 0;
    }
    return jobBeingParsed;
}

int parse_cmd_args(int argc, char **argv)
{
    if (argc == 1) {
        fprintf(stderr, BAD_USAGE_ERRM);
        exit(BAD_USAGE_ERRC);
    }
    
    int verboseMode = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], VERBOSE_FLAG) == 0) {
            if (i == 1) {
                verboseMode = 1;
            } else {
                fprintf(stderr, BAD_USAGE_ERRM);
                exit(BAD_USAGE_ERRC);
            }
        }
    }

    if (argc == 2 && verboseMode) {
        fprintf(stderr, BAD_USAGE_ERRM);
        exit(BAD_USAGE_ERRC);
    }
    
    return verboseMode;
}

FILE *open_job_file(char *fileAddr)
{
    FILE *jobFile = fopen(fileAddr, "r");

    if (jobFile == NULL) {
        fprintf(stderr, BAD_FILENAME_ERRM, fileAddr);
        exit(BAD_FILENAME_ERRC);
    } else {
        return jobFile;
    }
}

Job *init_new_job(void)
{
    Job *newJob = malloc(sizeof(Job));

    newJob->jobId = 0;
    
    newJob->cmd = malloc(1 * sizeof(char));
    newJob->cmd[0] = '\0';

    newJob->cmdArgs = malloc(1 * sizeof(char *));
    newJob->cmdArgs[0] = NULL;

    newJob->pipeFromName = malloc(1 * sizeof(char));
    newJob->pipeFromName[0] = '\0';

    newJob->pipeToName = malloc(1 * sizeof(char));
    newJob->pipeToName[0] = '\0';

    newJob->pipeFromLoc = -1;
    newJob->pipeToLoc = -1;

    newJob->timeout = 0;
    newJob->nextJob = NULL;

    newJob->jobPid = 0;

    return newJob;
}

int empty_line(char *line)
{
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] != '\t' || line[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

int validate_line(char **jobInfo, int numFields)
{
    if (numFields < MIN_FIELDS) {
        return 0;
    }

    // checking that input and output exist
    if (numFields >= MIN_FIELDS) {
        if (strlen(jobInfo[1]) <= 0 || strlen(jobInfo[2]) <= 0) {
            return 0;
        }
    }

    // checking that `ls,-,-,0,` is invalid
    if (numFields >= 5 && strlen(jobInfo[4]) < 1) {
        return 0;
    }

    // checking that the timeout field is valid
    if (numFields >= 4) {
        if (strlen(jobInfo[3]) > 0) {
            for (int i = 0; i < strlen(jobInfo[3]); i++) {
                if (!isdigit(jobInfo[3][i])) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

