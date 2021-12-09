// #define DEBUG

#ifndef JOB_STRUCT
#define JOB_STRUCT
#include "jobStruct.h"
#endif

#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2

/**
 * Generic skip_to_end function returns the last pointer in a linked list.
 *
 * Job *startOfList: the start of the list
 *
 * return: (Job*) the end of the list
 */
Job *skip_to_end(Job *startOfList);

/**
 * Counts the length of a null-terminated char* array.
 *
 * char **counted: the array to be counted
 *
 * return: (int) the number of elements in the array
 */
int count_fields(char **counted);

/**
 * Frees the given job, including its subfields.
 *
 * Job *unneeded: the "unneeded" job to be free'd
 */
void free_job(Job *unneeded);

/**
 * Counts the length of a linked list (ll) of jobs.
 *
 * Job *startOfList: start of linked list.
 */
int ll_length(Job *startOfList);

