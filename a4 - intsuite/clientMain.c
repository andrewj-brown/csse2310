#include "clientLibs.h"
#include "clientMain.h"

int main(int argc, char **argv) {
    comm_failure_handler();
    char *port;
    FILE *input = malloc(sizeof(FILE));
    int verbose = parse_cmd_args(argc, argv, &port, &input);
    
    int socket = connect_to_server(port);
    
    read_and_send_jobs(input, socket, verbose);
    
    return 0;
}

void comm_failure_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = recieve_sigpipe;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa, 0);
}

void recieve_sigpipe(int s) {
    fprintf(stderr, COMMS_ERROR);
    exit(3);
}

int parse_cmd_args(int argc, char **argv, char **port, FILE **input) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, BAD_USAGE_ERRM);
        exit(BAD_USAGE_ERRC);
    }
    
    int offset = 1;
    int retVal = 0;
    if (!strcmp(argv[offset], "-v")) {
        if (argc < 3) {
            fprintf(stderr, BAD_USAGE_ERRM);
            exit(BAD_USAGE_ERRC);
        }

        retVal = 1;
        offset++;
    }

    *port = argv[offset];
    offset++;

    // by auto-setting the input to stdin, the rest of the code is identical
    // for whatever FILE* it's given
    if ((argc == 4) || (argc == 3 && !retVal)) {
        *input = fopen(argv[offset], "r");
        if (*input == NULL) {
            fprintf(stderr, BAD_FILE_ERRM, argv[offset]);
            exit(BAD_FILE_ERRC);
        }
    } else {
        *input = stdin;
    }

    return retVal;
}

int connect_to_server(char *portNum) {
    struct addrinfo *ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    // failed to create socket implies bad portnum
    if (getaddrinfo(NULL, portNum, &hints, &ai)) {
        fprintf(stderr, BAD_PORT_ERRM, portNum);
        exit(BAD_PORT_ERRC);
    }

    int connectFd = socket(AF_INET, SOCK_STREAM, 0);

    int optVal = 1;
    if (setsockopt(connectFd, SOL_SOCKET, SO_REUSEADDR,
            &optVal, sizeof(int))) {
        // failed to set socket options
        // this failure is out-of-scope
        exit(-1);
    }

    if (connect(connectFd, (struct sockaddr *)ai->ai_addr,
            sizeof(struct sockaddr))) {
        // failed to connect implies no listener implies bad portnum
        fprintf(stderr, BAD_PORT_ERRM, portNum);
        exit(BAD_PORT_ERRC);
    }

    return connectFd;
}

void read_and_send_jobs(FILE *input, int socket, int verbose) {
    char *line;
    int lineNum = 0;
    
    while ((line = read_line(input))) {
        lineNum++;
        if (line[0] == COMMENT_IDENTIFIER || empty_line(line)) {
            free(line);
            continue;
        }
        
        struct ParsedJob *jobLine = malloc(sizeof(struct ParsedJob));
        if (validate_line(line, lineNum, jobLine)) {
            // note that because the first ',' was replaced by null, line is
            // now effectively a pointer to the first elem in a split array,
            // which is the function
            if (!server_check_function(line, socket)) {
                fprintf(stderr, BAD_JOB_INVALD, line, lineNum);
                continue;
            }
            
            jobLine->function = malloc((strlen(line) + 1) * sizeof(char));
            strcpy(jobLine->function, line);
            free(line);
            
            server_do_integral(jobLine, socket, verbose);
        } else {
            free(jobLine);
        }
    }
}

int empty_line(char *line) {
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] != '\t' || line[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

int contains_whitespace(char *string) {
    for (int i = 0; i < strlen(string); i++) {
        if (isspace(string[i])) {
            return 1;
        }
    }
    return 0;
}

int string_occurrence(char *string, char check) {
    int count = 0;
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == check) {
            count++;
        }
    }
    return count;
}

int validate_line(char *line, int lineNum, struct ParsedJob *jobLine) {
    if (string_occurrence(line, ',') != 4) {
        fprintf(stderr, BAD_JOB_SYNTAX, lineNum);
        return 0;
    }
    char **splitLine = split_by_char(line, ',', 6);
    
    char err;
    if (sscanf(splitLine[1], "%lf%c", &jobLine->lower, &err) != 1) {
        fprintf(stderr, BAD_JOB_SYNTAX, lineNum);
        return 0;
    } else if (sscanf(splitLine[2], "%lf%c", &jobLine->upper, &err) != 1) {
        fprintf(stderr, BAD_JOB_SYNTAX, lineNum);
        return 0;
    } else if ((sscanf(splitLine[3], "%d%c", &jobLine->segments, &err) != 1)
            || overflow(splitLine[3])) {
        fprintf(stderr, BAD_JOB_SYNTAX, lineNum);
        return 0;
    } else if ((sscanf(splitLine[4], "%d%c", &jobLine->threads, &err) != 1)
            || overflow(splitLine[4])) {
        fprintf(stderr, BAD_JOB_SYNTAX, lineNum);
        return 0;
    } else if (contains_whitespace(splitLine[0])) {
        fprintf(stderr, BAD_JOB_SPACES, lineNum);
        return 0;
    } else if (jobLine->upper <= jobLine->lower) {
        fprintf(stderr, BAD_JOB_BOUNDS, lineNum);
        return 0;
    } else if (jobLine->segments <= 0) {
        fprintf(stderr, BAD_JOB_SEGMNT, lineNum);
        return 0;
    } else if (jobLine->threads <= 0) {
        fprintf(stderr, BAD_JOB_THREAD, lineNum);
        return 0;
    } else if (jobLine->segments % jobLine->threads != 0) {
        fprintf(stderr, BAD_JOB_SPLITS, lineNum);
        return 0;
    }

    return 1;
}

int overflow(char *string) {
    char *drop;
    long val = strtol(string, &drop, 10);
    
    if (val >= INT_MAX || errno == ERANGE) {
        return 1;
    } else {
        return 0;
    }
}

int server_check_function(char *function, int socket) {
    // print request to socket, and enter a read loop awaiting response
    char *request = calloc(128, sizeof(char));
    sprintf(request, HTTP_VALIDATION_REQUEST, function);
    write(socket, request, strlen(request));
    free(request);
   
    char *buffer = calloc(1024, sizeof(char));
    char *response = calloc(1, sizeof(char));
    ssize_t numBytesRead;
    int reqlen = 0;
    
    while ((numBytesRead = read(socket, buffer, 1023)) > 0) {
        if (numBytesRead < 0) {
            fprintf(stderr, COMMS_ERROR);
            exit(3);
        }
        reqlen += numBytesRead;

        response = realloc(response, (reqlen + 1) * sizeof(char));
        strcat(response, buffer);
        memset(buffer, '\0', 1024);

        int status;
        char *statusExplanation;
        HttpHeader **headers;
        char *body;
        int valid = parse_HTTP_response(response, strlen(response), &status,
                &statusExplanation, &headers, &body);
        // negative means malformed, zero means keep reading, positive means
        // well-formed, so we handle it
        if (valid < 0) {
            fprintf(stderr, COMMS_ERROR);
            exit(3);
        } else if (valid == 0) {
            continue;
        } else {
            if (status == 200) {
                return 1;
            } else if (status == 400) {
                return 0;
            } else {
                fprintf(stderr, COMMS_ERROR);
                exit(3);
            }
        }
    }
    // if for some godforsaken reason the loop breaks, something's gone wrong
    fprintf(stderr, COMMS_ERROR);
    exit(3);
}

void server_do_integral(struct ParsedJob *jobLine, int socket, int verbose) {
    // function structure basically identical to server_check_function above
    char *request = calloc(256, sizeof(char));
    sprintf(request, HTTP_INTEGRATION_REQUEST, jobLine->lower, jobLine->upper,
            jobLine->segments, jobLine->threads, jobLine->function);
    write(socket, request, strlen(request));
    if (verbose) {
        write(socket, VERBOSE_HEADER, 16);
    }
    write(socket, "\r\n", 2);
    char *buffer = calloc(1024, sizeof(char));
    char *response = calloc(1, sizeof(char));
    ssize_t numBytesRead;
    int reqlen = 0;
    while ((numBytesRead = read(socket, buffer, 1023)) > 0) {
        if (numBytesRead < 0) {
            fprintf(stderr, COMMS_ERROR);
            exit(3);
        }
        reqlen += numBytesRead;
        
        response = realloc(response, (reqlen + 1) * sizeof(char));
        strcat(response, buffer);
        memset(buffer, '\0', 1024);
        
        int status;
        char *statusExplanation;
        HttpHeader **headers;
        char *body;
        int valid = parse_HTTP_response(response, strlen(response), &status,
                &statusExplanation, &headers, &body);
        if (valid < 0) {
            fprintf(stderr, COMMS_ERROR);
            exit(3);
        } else if (valid == 0) {
            continue;
        } else {
            if (status == 200) {
                print_response(body, jobLine);
                return;
            } else if (status == 400) {
                fprintf(stderr, INTEGRATION_FAILED);
                return;
            } else {
                fprintf(stderr, COMMS_ERROR);
                exit(3);
            }
        }
    }
}

int get_content_length(HttpHeader **headers) {
    int i = 0;
    while (headers[i]) {
        if (!strcmp((headers[i])->name, "Content-Length")) {
            return atoi((headers[i])->value);
        }
        
        i++;
    }
    return -1;
}

void print_response(char *body, struct ParsedJob *jobLine) {
    int i = 0;
    double result;
    char **lines = split_by_char(body, '\n', 0);
    while (lines[i] != NULL) {
        if (sscanf(lines[i], "%lf", &result) == 1) {
            fprintf(stdout, RESULT_OUTPUT, jobLine->function, jobLine->lower,
                    jobLine->upper, result);
            return;
        } else {
            fprintf(stdout, "%s\n", lines[i]);
        }
        i++;
    }
}
