#include "serverLib.h"
#include "serverMain.h"
#include "serverComp.h"
#include "serverHup.h"

int main(int argc, char **argv) {
    int portnum, maxThreads;
    parse_cmd_args(argc, argv, &portnum, &maxThreads);
    
    int serverFd = open_listener(argv[1]);

    struct SignalInformation *progStats = register_sig_handler();

    process_incoming_connections(serverFd, maxThreads, progStats);

    return 0;
}

int open_listener(char *port) {
    struct addrinfo *ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;        // listen fron amy IP

    if (getaddrinfo(NULL, port, &hints, &ai)) {
        // failed to determine address
        // this error is out-of-scope
        exit(-1);
    }

    int listenFd = socket(AF_INET, SOCK_STREAM, 0);

    int optVal = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR,
            &optVal, sizeof(int))) {
        // failed to set socket options
        // this error is also out-of-scope
        exit(-1);
    }

    if (bind(listenFd, (struct sockaddr *)ai->ai_addr, 
            sizeof(struct sockaddr))) {
        // failed to bind implies bad portnum
        fprintf(stderr, BAD_PORT_ERRM);
        exit(BAD_PORT_ERRC);
    } 

    if (listen(listenFd, 10) < 0) {  // Up to 10 connection requests can queue
        // failed to listen
        // this error is also out-of-scope
        exit(-1);
    }

    // print the actual opened port
    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    socklen_t addrLen = sizeof(*addr);
    memset(addr, 0, addrLen);
    getsockname(listenFd, (struct sockaddr *)addr, &addrLen);
    fprintf(stderr, "%d\n", ntohs(addr->sin_port));
    free(addr);

    return listenFd;
}

void parse_cmd_args(int argc, char **argv, int *portNum, int *maxThreads) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, BAD_USAGE_ERRM);
        exit(BAD_USAGE_ERRC);
    }
    
    int a, b;
    *portNum = atoi(argv[1]);
    // validate portnum
    if (*portNum < 0 || *portNum > 65535 ||
            (*portNum == 0 && argv[1][0] != '0') ||
            (sscanf(argv[1], "%d%c", &a, (char *)&b) > 1)) {
        fprintf(stderr, BAD_USAGE_ERRM);
        exit(BAD_USAGE_ERRC);
    }
    
    if (argc == 3) {
        *maxThreads = atoi(argv[2]);
        if (*maxThreads < 1) {
            fprintf(stderr, BAD_USAGE_ERRM);
            exit(BAD_USAGE_ERRC);
        }
    } else {
        *maxThreads = -1;
    }

    return;
}

void process_incoming_connections(int listenerSocket, int maxThreads,
        struct SignalInformation *progStats) {
    int handlerSocket;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    
    // initialise thread semaphore *if needed*
    sem_t *threadLimiter;
    if (maxThreads > 0) {
        threadLimiter = malloc(sizeof(sem_t));
        sem_init(threadLimiter, 0, maxThreads);
    } else {
        threadLimiter = NULL;
    }

    // main "wait for incoming connections" loop
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);

        handlerSocket = accept(listenerSocket, (struct sockaddr *)&fromAddr,
                &fromAddrSize);
        if (handlerSocket < 0) {
            exit(-1);
        }

        struct ClientThreadArgs *clientArgs = malloc(sizeof(
                struct ClientThreadArgs));
        clientArgs->socket = handlerSocket;
        clientArgs->threadLimiter = threadLimiter;
        clientArgs->progStats = progStats;

        pthread_t threadId;
        pthread_create(&threadId, NULL, client_thread_handler,
                (void *)clientArgs);

        pthread_detach(threadId);
    }
    // out here, the program is exiting. Should never happen.
    if (threadLimiter) {
        sem_destroy(threadLimiter);
    }
}

void *client_thread_handler(void *threadArgument) {
    char *request = calloc(1, sizeof(char));
    char *buffer = calloc(1024, sizeof(char));
    ssize_t numBytesRead;    
    int handlerSocket = ((struct ClientThreadArgs *)threadArgument)->socket;
    sem_t *threadLimiter = ((struct ClientThreadArgs *)
            threadArgument)->threadLimiter;
    struct SignalInformation *progStats = ((struct ClientThreadArgs *)
            threadArgument)->progStats;
    free(threadArgument);

    increment_active_clients(progStats);

    struct HttpRequest *parsedRequest = malloc(sizeof(struct HttpRequest));
    int reqlen = 0; 
    
    while ((numBytesRead = read(handlerSocket, buffer, 1023)) > 0) {
        reqlen += numBytesRead;

        request = realloc(request, (reqlen + 1) * sizeof(char));
        strcat(request, buffer);
        memset(buffer, '\0', 1024);

        int valid = parse_HTTP_request(request, reqlen,
                &(parsedRequest->method), &(parsedRequest->address),
                &(parsedRequest->headers), &(parsedRequest->body));

        if (valid < 0) {
            break;
        } else if (valid > 0) {
            handle_request(parsedRequest, handlerSocket, threadLimiter,
                progStats);
            
            reqlen = 0;
            free(request);
            request = calloc(1, sizeof(char));
            free_http_request(parsedRequest);
            // we *need* to free this in here because there might be more
            // requests incoming. and they need the space.
        } else {
            free(parsedRequest);
        }

        parsedRequest = malloc(sizeof(struct HttpRequest));
    }
    
    close(handlerSocket);
    decrement_active_clients(progStats);
    pthread_exit(NULL);
}

void free_http_request(struct HttpRequest *freed) {
    if (freed->method) {
        free(freed->method);
    }
    if (freed->address) {
        free(freed->address);
    }
    if (freed->headers) {
        free_array_of_headers(freed->headers);
    }
    if (freed->body) {
        free(freed->body);
    }
    free(freed);
}
