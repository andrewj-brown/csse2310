#include "clientLibs.h"

#ifndef C_MAIN
#define C_MAIN

#define BAD_USAGE_ERRM "Usage: intclient [-v] portnum [jobfile]\n"
#define BAD_USAGE_ERRC 1
#define BAD_FILE_ERRM "intclient: unable to open \"%s\" for reading\n"
#define BAD_FILE_ERRC 4
#define BAD_PORT_ERRM "intclient: unable to connect to port %s\n"
#define BAD_PORT_ERRC 2

#define BAD_JOB_SYNTAX "intclient: syntax error on line %i\n"
#define BAD_JOB_SPACES "intclient: spaces not permitted in expression (line \
%i)\n"
#define BAD_JOB_BOUNDS "intclient: upper bound must be greater than lower \
bound (line %i)\n"
#define BAD_JOB_SEGMNT "intclient: segments must be a positive integer (line \
%i)\n"
#define BAD_JOB_THREAD "intclient: threads must be a positive integer (line \
%i)\n"
#define BAD_JOB_SPLITS "intclient: segments must be an integer multiple of \
threads (line %i)\n"
#define BAD_JOB_INVALD "intclient: bad expression \"%s\" (line %i)\n"

#define COMMS_ERROR "intclient: communications error\n"
#define INTEGRATION_FAILED "intclient: integration failed\n"

#define HTTP_VALIDATION_REQUEST "GET /validate/%s HTTP/1.0\r\n\r\n"
#define HTTP_INTEGRATION_REQUEST "GET /integrate/%lf/%lf/%d/%d/%s HTTP/1.0\r\n"
#define VERBOSE_HEADER "X-Verbose: yes\r\n"

#define RESULT_OUTPUT "The integral of %s from %lf to %lf is %lf\n"

#define COMMENT_IDENTIFIER '#'

/**
 * Struct for storing a parsed job line.
 */
struct ParsedJob {
    char *function;
    double lower;
    double upper;
    int segments;
    int threads;
};

/**
 * Sends an integration request to the server, parses the response, and sends
 * it to stdout.
 *
 * struct ParsedJob *jobLine: the job to be sent to the server
 * int socket: the socket to send the request on
 * int verbose: whether or not to set verbose
 */
void server_do_integral(struct ParsedJob *jobLine, int socket, int verbose);

/**
 * Parses command arguments, saves the outgoing portnum into port and
 * any relevant jobfile into jobFile, and returns the status of the verbose
 * flag.
 *
 * int argc: number of command args
 * char **argv: command args
 * int *port: address to be filled with port
 * FILE **input: address to be filled with a jobfile
 *
 * return (int): verbose flag
 */
int parse_cmd_args(int argc, char **argv, char **port, FILE **input);

/**
 * Opens (or attempts to open) a connection with localhost:<portNum>.
 *
 * char *portNum: unparsed port number to try connect to
 *
 * return (int): file descriptor of a socket connected to a server
 */
int connect_to_server(char *portNum);

/**
 * The main "logic" of intclient. Reads job lines from input, parses them, and
 * sends requests to socket. Also handles responses and output, through other 
 * functions.
 *
 * FILE *input: input file pointer
 * int socket: fd of connected socket
 * int verbose: the status of the verbose flag
 */
void read_and_send_jobs(FILE *input, int socket, int verbose);

/**
 * Checks if a line is empty.
 *
 * char *line: line to check
 *
 * return (int): 1 if the line is empty, else 0
 */
int empty_line(char *line);

/**
 * Validates a given line.
 *
 * char *line: line to check
 * int *lineNum: number of this line within the file
 * struct ParsedJob *jobLine: address to return the parsed job into
 *
 * return (int): 1 if the line is valid, else 0
 */
int validate_line(char *line, int lineNum, struct ParsedJob *jobLine);

/**
 * Check if the given string contains any whitespace characters.
 *
 * char *string: string to check
 *
 * return (int): 1 if the string contains whitespace, else 0
 */
int contains_whitespace(char *string);

/**
 * Send a validation request to the server for a given function.
 * 
 * char *function: function to validate
 * int socket: the socket that's connected to the server
 *
 * return (int): 1 if the function is good, otherwise 0.
 */
int server_check_function(char *function, int socket);

/**
 * Detects whether the given string causes integer overflow.
 *
 * char *string: string to check
 *
 * return (int): 1 if overflow occurs, else 0.
 */
int overflow(char *string);

/**
 * Finds the content length header and returns its value.
 *
 * HttpHeader **headers: array of http headers
 *
 * return (int): value of content-length header or -1 if not present
 */
int get_content_length(HttpHeader **headers);

/**
 * Prints a http response to stdout.
 *
 * char *body: body of http response
 * struct ParsedJob *jobLine: job result to print
 */
void print_response(char *body, struct ParsedJob *jobLine);

/**
 * Counts the occurrences of a character in a string.
 *
 * char *string: string that we count occurences in
 * char check: character that we're looking for
 *
 * return int: the number of times check occurs in string.
 */
int string_occurrence(char *string, char check);

/**
 * Registers the "backup" signal handler for comms failure. 
 */
void comm_failure_handler(void);

/**
 * "Gracefully" exits on SIGPIPE.
 * Really, it just calls exit(3), as a backup incase the other checks fail
 *
 * int s: number of the signal we recieved; generally irrelevant here.
 */
void recieve_sigpipe(int s);

#endif
