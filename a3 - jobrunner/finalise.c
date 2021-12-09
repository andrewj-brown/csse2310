#ifndef LIBRARIES
#define LIBRARIES
#include <csse2310a3.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

#ifndef FINAL_HEADER
#define FINAL_HEADER
#include "finalise.h"
#endif

/**
 * killSignal boolean flag for if the SIGHUP kill-all signal has been sent
 */
bool killSignal = false;

void check_all_jobs_timer(Job *overallFirstJob)
{
    int timerSeconds = 0;
    while (1) {
        if (killSignal) {
            kill_all(overallFirstJob);
        }

        overallFirstJob = check_all_jobs_loop(overallFirstJob);
        job_timeouts(overallFirstJob, timerSeconds);
        
        sleep(1);
        timerSeconds++;

        if (!overallFirstJob) {
            break;
        }
    }
}

void setup_signal_kill_all(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = kill_flag;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, 0);
}

void kill_flag(int s)
{
    killSignal = true;
}

void kill_all(Job *jobIterator)
{
    #ifdef DEBUG
    fprintf(stderr, "Killing all jobs...\n");
    #endif
    while (jobIterator) {
        kill(jobIterator->jobPid, SIGKILL);
        jobIterator = jobIterator->nextJob;
    }
}

void job_timeouts(Job *jobIterator, int timerSeconds)
{
    while (jobIterator) {
        if (jobIterator->timeout <= timerSeconds &&
                jobIterator->timeout != 0) {
            if (jobIterator->timeout > 0) {
                #ifdef DEBUG
                fprintf(stderr, "First timeout on job %i\n",
                        jobIterator->jobId);
                #endif

                kill(jobIterator->jobPid, SIGABRT);
                jobIterator->timeout = -1;
            } else {
                #ifdef DEBUG
                fprintf(stderr, "Second timeout on job %i\n",
                        jobIterator->jobId);
                #endif

                kill(jobIterator->jobPid, SIGKILL);
            }
        }

        jobIterator = jobIterator->nextJob;
    }

    return;
}

Job *check_all_jobs_loop(Job *jobIterator)
{
    Job *prevJob = NULL;
    Job *firstJob = jobIterator;

    while (jobIterator) {
        int childStatus = 0;
        int pid = waitpid(jobIterator->jobPid, &childStatus, WNOHANG);
        
        if (pid > 0) {
            if (WIFEXITED(childStatus)) {
                fprintf(stderr, JOB_EXITED_REPORT, jobIterator->jobId,
                        WEXITSTATUS(childStatus));
                fflush(stderr);
                
                if (!prevJob) {
                    firstJob = jobIterator->nextJob;
                } else {
                    prevJob->nextJob = jobIterator->nextJob;
                }
            } else if (WIFSIGNALED(childStatus)) {
                fprintf(stderr, JOB_TERMINATED_REPORT, jobIterator->jobId,
                        WTERMSIG(childStatus));
                fflush(stderr);
    
                if (!prevJob) {
                    firstJob = jobIterator->nextJob;
                } else {
                    prevJob->nextJob = jobIterator->nextJob;
                }
            }
        } else if (pid == 0) {
            prevJob = jobIterator;
        }

        jobIterator = jobIterator->nextJob;
    
    }
    
    return firstJob;
}

void cleanup(Job *jobIterator)
{
    while (jobIterator) {
        Job *nextJob = jobIterator->nextJob;

        free_job(jobIterator);

        jobIterator = nextJob;        
    }
}

