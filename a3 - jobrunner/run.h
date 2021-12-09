#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

#define BAD_FAILED_EXEC_ERRC 255
#define DEAD_END "/dev/null"

/**
 * Main code "running" process that handles everything between "list is done"
 * and the fork() call.
 *
 * Job *firstJobInList: first job in linked list of jobs
 */
void run_jobs(Job *firstJobInList);

/**
 * Continues post-fork as a child. Handles exec'ing functions, duping fds,
 * and generall running of the individual job passed.
 *
 * Job *firstJobInList: first job in list of jobs, to delete them all
 * Job *myJob: the job that this child is running.
 */
void continue_as_child(Job *firstJobInList, Job *myJob); 

/**
 * Continues post-fork as a parent. Handles continuing the loop, free'ing 
 * memory, and general cleanup as the program heads into finalisation.
 *
 * Job *childJob: the child job that just got run
 *
 * return: (Job*) the next job that the run_jobs loop should handle
 */
Job *continue_as_parent(Job *childJob);

/**
 * Deletes and closes everything that a child doesn't need - i.e. all the 
 * other jobs and job-file-descriptor-outputs stored in the master list
 *
 * Job *firstJobInList: first job in master list
 * Job *myJob: the job for this child
 */
void clear_useless_information(Job *firstJobInList, Job *myJob);

