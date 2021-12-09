#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

#define JOB_EXITED_REPORT "Job %i exited with status %i\n"
#define JOB_TERMINATED_REPORT "Job %i terminated with signal %i\n"

/**
 * Starts the job-checkup timer loop, which uses the below function to check
 * jobs once per second.
 *
 * Job *firstJobInList: first job in master list of jobs.
 */
void check_all_jobs_timer(Job *firstJobInList);

/**
 * Starts one job-checkup loop, which iterates over the linked list of jobs
 * and reports back on exits or signals.
 *
 * Job *jobIterator: first job in master list of jobs.
 *
 * return: (Job*) new first job, as we remove jobs from the list when they exit
 */
Job *check_all_jobs_loop(Job *jobIterator);

/**
 * Handles killing jobs that have timed out, and jobs that refuse to die.
 *
 * Job *jobIterator: first job in master list of jobs.
 * int timerSeconds: number of seconds the program has been running for.
 */
void job_timeouts(Job *jobIterator, int timerSeconds);

/**
 * Real "signal handler" for the SIGHUP signal, sets the killSignal global
 * boolean.
 *
 * int s: an integer that helps deal with multiple signals, irrelevant here
 */
void kill_flag(int s);

/**
 * Kills all the jobs, usef for when the killSignal global boolean is set.
 *
 * Job *jobIterator: first job in master list of jobs.
 */
void kill_all(Job *jobIterator);

/**
 * Sets up the signal handler for SIGHUP, which is the above kill_signal
 * function.
 */
void setup_signal_kill_all(void);

/**
 * Cleans up after everything is done. 
 *
 * Job *jobIterator: first job in master list of jobs.
 */
void cleanup(Job *jobIterator);

