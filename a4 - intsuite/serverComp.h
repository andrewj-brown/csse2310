#include "serverLib.h"
#include "serverHttpStruct.h"
#include "serverHupStruct.h"

#ifndef S_COMP
#define S_COMP

#define DO_VAL "validate"
#define DO_INT "integrate"

#define INVALID_VAL 0
#define VALIDATION 1
#define INTEGRATION 2
#define INVALID_INT 3

#define VERBOSE_HEADER_NAME "X-Verbose"
#define VERBOSE_HEADER_VAL "yes"

/**
 * Parameters for an integration to be performed by a computation thread.
 */
struct IntegrationParams {
    int num;
    char *function;
    double lower;
    double upper;
    double width;
    int segments;
    sem_t *threadLimiter;
};

/**
 * Struct for storing the result of a single computation thread. Includes -v
 * fields for potential output.
 */
struct ThreadResult {
    int num;
    double lower;
    double upper;
    double result;
};

/**
 * Sends a http message "400: Bad Request".
 *
 * int socket: the socket to send the response to.
 */
void respond_invalid_request(int socket);

/**
 * Sends a http message "200: OK".
 *
 * int socket: the socket to send the response to.
 */
void respond_valid_request(int socket);

/**
 * Handles a well-formed (although possibly not valid) HTTP request.
 *
 * struct HttpRequest *parsedRequest: a well-formed, parsed HTTP request
 * int clientSocket: file descriptor of client socket that sent the request
 * sem_t *threadLimiter: computation-thread limiting semaphore
 */
void handle_request(struct HttpRequest *parsedRequest, int clientSocket,
        sem_t *threadLimiter, struct SignalInformation *progStats);

/**
 * Validates a well-formed HTTP request, ensures it's something we can do,
 * checks headers, etc etc. Calls invalid_request() if it's not.
 *
 * struct HttpRequest *parsedRequest: a well-formed, parsed HTTP request
 *
 * return int: 1 for validate, -1 for integrate, and 0 for invalid.
 */
int valid_request(struct HttpRequest *parsedRequest);

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
 * Integrates a request-to-integrate (assumed to be valid) over multiple 
 * threads. Handles -v mode output.
 *
 * struct HttpRequest *parsedRequest: a well-formed, valid HTTP request
 * int clientSocket: file descriptor of socket for this client
 * sem_t *threadLimiter: computation-thread limiting semaphore
 * int verbose: verbose mode status
 * struct SignalInformation *progStats: SIGHUP program stats
 */
void integrate_multithreaded(struct HttpRequest *parsedRequest,
        int clientSocket, sem_t *threadLimiter, int verbose, 
        struct SignalInformation *progStats);

/**
 * Handles a computation thread; uses integration parameters to integrate
 * a function and then returns relevant information.
 *
 * void *threadArgument: see IntegrationParams above.
 */
void *computation_thread_handler(void *threadArgument);

/**
 * Finalises computation threads, waits for all of them, aggregates results
 * and sends them to the client. 
 *
 * pthread_t *threadIds: the thread IDs of the computation threads to check
 * int clientSocket: socket to respond to client on
 * int verbose: verbose mode status
 * struct SignalInformation *progStats: SIGHUP program stats
 */
void finalise_integration(pthread_t *threadIds, int clientSocket, int verbose,
        struct SignalInformation *progStats);

/**
 * The function that does the trapezoidal estimation of the integral.
 *
 * te_expr *expr: expression to be evaluated;
 * double *x: pointer to "expression" value for the function
 * double lower: lower bound
 * double upper: upper bound
 * int segments: number of segments to do.
 *
 * return (double): trapezoidal estimation of the integral based on params
 */
double do_math(te_expr *expr, double *x, double lower, double upper,
        int segments);

/**
 * Checks that the given split adddress is only double or integers, depending
 * on their positions relative to a /integrate/? request.
 *
 * char **intParams: split address
 *
 * return (int): 0 if the address fails checks, otherwise 1
 */
int check_fields_only_nums(char **intParams);

/**
 * Validates a given expression with te.
 *
 * char *expression: string of the expression
 *
 * return (int): 1 if the expression is good, otherwise 0
 */
int te_validate(char *expression);

/**
 * Parses an array of http headers checking that X-Verbose is set.
 * 
 * HttpHeader **headers: headers of the request
 *
 * return (int): 1 if X-Verbose is present, otherwise 0
 */
int check_verbose(HttpHeader **headers);

#endif
