#include "serverLib.h"
#include "serverHttpStruct.h"
#include "serverHupStruct.h"

#ifndef S_MAIN
#define S_MAIN

#define MAX_HOST_NAME_LEN 128

#define BAD_USAGE_ERRM "Usage: intserver portnum [maxthreads]\n"
#define BAD_USAGE_ERRC 1
#define BAD_PORT_ERRM "intserver: unable to open socket for listening\n"
#define BAD_PORT_ERRC 3

/**
 * Arguments that get passed to client threads.
 */
struct ClientThreadArgs {
    int socket;
    sem_t *threadLimiter;
    struct SignalInformation *progStats;
};

/**
 * Parses intserver command args into the two provided int*s. Handles invalid
 * command lines.
 *
 * int argc: number of command line args
 * char **argv: command line args
 * int *portnum: output of portnum argument
 * int *max_threads: output of maxthreads argument, or -1 if not supplied
 */
void parse_cmd_args(int argc, char **argv, int *portNum, int *maxThreads);

/**
 * Opens a socket fd to listen for incoming connections.
 *
 * This code is largely reused from Lecture 18 - `server-multithreaded.c`
 *
 * char *port: port number to listen on
 */
int open_listener(char *port);

/**
 * Processes incoming connections and spawns a new thread for each one.
 *
 * This code is semi-reused from Lecture 18 - `server-multithreaded.c`
 *
 * int socket: open file descriptor of a port awaiting connections
 * int maxThreads: overall maximum thread limit of intserver
 */
void process_incoming_connections(int socket, int maxThreads, 
        struct SignalInformation *progStats);

/**
 * Handles one client connection. Spawns new threads for computation and 
 * handles data aggregation and mutual-exclusion.
 *
 * This code is somewhat inspired by Lecture 18 - `server-multithreaded.c`
 *
 * void *threadArgument: see struct clientThreadArgs above.
 * ret (void *): a pthread requirement; this function only ever returns NULL.
 */
void *client_thread_handler(void *threadArgument);

/**
 * Frees a HttpRequest struct, when all fields may or may not have been filled
 * by parse_HTTP_request.
 *
 * struct HttpRequest *freed: the HttpRequest to be freed
 */
void free_http_request(struct HttpRequest *freed);

#endif
