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

#define BAD_READ_FILENAME "Unable to open \"%s\" for reading\n"
#define BAD_WRITE_FILENAME "Unable to open \"%s\" for writing\n"
#define BAD_PIPE_USAGE "Invalid pipe usage \"%s\"\n"

#define BAD_NO_JOBS_ERRM "jobrunner: no runnable jobs\n"
#define BAD_NO_JOBS_ERRC 4
#define BAD_JOBLINE_ERRM "jobrunner: invalid job specification on \
line %i of \"%s\"\n"
#define BAD_JOBLINE_ERRC 3
#define BAD_FILENAME_ERRM "jobrunner: file \"%s\" can not be opened\n"
#define BAD_FILENAME_ERRC 2
#define BAD_USAGE_ERRM "Usage: jobrunner [-v] jobfile [jobfile ...]\n"
#define BAD_USAGE_ERRC 1

#define DEFAULT_IO "-"
#define VERBOSE_FLAG "-v"
#define COMMENT_IDENTIFIER '#'
#define PIPE_IDENTIFIER '@'

#define MIN_FIELDS 3
#define PERMS_ALL 0777

/**
 * Creates a new "empty" job with default values for all fields.
 *
 * return: (Job) a new empty job
 */
Job *init_new_job(void);

/**
 * Loops over a list of jobs and assigns them all an "id" - an incremented
 * integer.
 *
 * Job *firstJobInList: pointer to first job in master list of jobs
 */
void assign_job_ids(Job *firstJobInList);

/**
 * Opens a new pointer to a jobfile. Handles missing jobfiles / bad jobfile
 * names.
 *
 * char *fileAddr: the location to look for the jobfile.
 *
 * return: (FILE *) an open jobfile pointer
 */
FILE *open_job_file(char *fileAddr);

/**
 * Parses commandline args looking for the -v flag.
 * Handles improper argument use.
 *
 * int argc: number of commandline args
 * char **argv: commandline args
 *
 * return: (int) the status of the -v flag (1 for yes, 0 for no)
 */
int parse_cmd_args(int argc, char **argv);

/**
 * Reads jobFile from from only a name, using open_job_file to open it and
 * given functions to parse.
 *
 * char *fileAddr: string address of the jobfile to be parsed
 *
 * return: (*Job) pointer to first job in a linked list of jobs for this file
 */
Job *parse_job_file(char *fileAddr);

/**
 * Reads every jobfile using parse_job_file. Handles combining per-file lists
 * into a master list of every job to be run. Also uses the checking functions
 * to cull the master list down to runnable jobs.
 *
 * int argc: number of commandline args
 * char **argv: commandline args
 * int verboseMode: whether or not the verboseMode flag was supplied
 *
 * return (*Job) pointer to first job in master linked list of jobs
 */
Job *parse_job_files(int argc, char **argv, int verboseMode);

/**
 * Creates a job struct from one line. Handles invalid lines and sets up pipe
 * end checking.
 *
 * char **jobInfo: jobfile line parsed by splitByCommas
 * int numFields: the number of fields the line has
 * int lineNum: the line number of this job in the jobfile
 * char *fileAddr: the file address of the current jobfile
 *
 * return: (Job*) pointer to a job with all the info from the jobfile
 */
Job *create_job_from_line(char **jobInfo, int numFields, int lineNum,
        char *fileAddr);

/**
 * Checks that the filenames provided to the program are available.
 *
 * Job *checkJob: the job whose filenames get checked
 *
 * return (int): 1 if the filenames are fine, else 0
 */
int good_filenames(Job *checkJob);

/**
 * Checks that the pipes in this program are well-formed, i.e. have exactly
 * one input and output end.
 *
 * Job *checkJob: the job whose pipes get checked
 * Job *firstRunnableJob: the first job in the overall list, to loop over
 * char **alreadyPrinted: pipes that have already been notified as invalid
 * bool firstPass: whether or not this is the first pipe-check that we've done
 *
 * return (int): 1 if the pipes are fine, else 0
 */
int good_pipes(Job *checkJob, Job *firstRunnableJob, char **alreadyPrinted,
        bool firstPass);

/**
 * Culls the linked list of jobs down to only ones with openable files.
 *
 * Job *firstJobInList: pointer to first job in master list
 *
 * return (Job*): pointer to first job in culled list
 */
Job *limit_openable_files(Job *firstJobInList);

/**
 * Culls the linked list of jobs down to only ones with valid pipes.
 *
 * I would love to have this function and the related limit_openable_files
 * be only one function, but unfortunately, the spec says all the file problems
 * have to come before pipe problems, they can't be interlaced.
 *
 * Job *firstJobInList: pointer to first job in master list
 * bool firstPass: whether or not this is the first pipe-check that we've done
 *
 * return (Job*): pointer to first job in culled list
 */
Job *limit_workable_pipes(Job *firstJobInList, bool firstPass);

/**
 * Checks if a line is empty; designed to be used as if (emptyLine(line))
 *
 * char *line: line being checked for emptiness
 *
 * return: (int) 1 if the line is empty else 0
 */
int empty_line(char *line);

/**
 * Opens a pipe and gives the correct ends to the given jobs.
 *
 * Job *fromJob: job that will hold the write end
 * Job *toJob: job that will hold the read end
 */
void open_pipes(Job *fromJob, Job *toJob);

/**
 * Validates a given line, checks that necessary fields are present and
 * all given values are valid.
 *
 * char **jobInfo: a job line that's been parsed by splitByCommas
 * int numFields: the number of fields the line has
 *
 * return: (int) 1 if the line is valid else 0
 */
int validate_line(char **jobInfo, int numFields);

/**
 * Prints the "job table" for the program.
 *
 * Job *printJob: first job in list of jobs to be printed.
 */
void print_job_table(Job *printJob);

/**
 * Finds the "read end" that matches this job's "write end".
 * 
 * Job *jobIterator: first job in linked list of jobs, so we can loop over it
 * Job *checkJob: the job who's read end is getting validated
 *
 * return: (Job*) the job with the matching end, or NULL if the pipe is invalid
 */
Job *match_from_end(Job *jobIterator, Job *checkJob);

/**
 * Finds the "write end" that matches this job's "read end".
 *
 * Job *jobIterator: first job in linked list of jobs, so we can loop over it
 * Job *checkJob: the job who's write end is getting validated
 *
 * return: (Job*) the job with the matching end, or NULL if the pipe is invalid
 */
Job *match_to_end(Job *jobIterator, Job *checkJob);

/**
 * Checks for two read ends of the same pipe.
 *
 * Job *jobIterator: first job in linked list of jobs
 * Job *checkJob: job that's getting checked for duplicate read ends
 *
 * return: (int) 1 if there's two read ends else 0
 */
int two_read_ends(Job *jobIterator, Job *checkJob);

/**
 * Checks for two write ends of the same pipe.
 *
 * Job *jobIterator: first job in linked list of jobs
 * Job *checkJob: job that's getting checked for duplicate write ends
 *
 * return: (int) 1 if there's two write ends else 0
 */
int two_write_ends(Job *jobIterator, Job *checkJob);

/**
 * Prints an invalid pipe usage message, but only once. If a pipeName
 * is passed to this function that has already been printed, the function
 * does nothing.
 *
 * char *pipeName: name of pipe to be printed
 * char **alreadyPrinted: names of pipes we've already printed
 * bool firstPass: whether or not this is the first pipe-check we've done
 */
void print_pipe_once(char *pipeName, char **alreadyPrinted, bool firstPass);

