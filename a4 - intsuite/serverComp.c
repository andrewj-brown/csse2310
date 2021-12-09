#include "serverLib.h"
#include "serverHttpStruct.h"
#include "serverComp.h"
#include "serverHup.h"
#include "serverHupStruct.h"

void respond_invalid_request(int socket) {
    #ifdef DEBUG
    fprintf(stderr, "Responding 400 on socket %i\n\n", socket);
    #endif

    char *resp = construct_HTTP_response(400, "Bad Request", NULL, NULL);
    write(socket, resp, strlen(resp));
    free(resp);
}

void respond_valid_request(int socket) {
    #ifdef DEBUG 
    fprintf(stderr, "Responding 200 on socket %i\n\n", socket);
    #endif

    char *resp = construct_HTTP_response(200, "OK", NULL, NULL);
    write(socket, resp, strlen(resp));
    free(resp);
}

int valid_request(struct HttpRequest *parsedRequest) {
    if (strcmp(parsedRequest->method, "GET")) {
        return INVALID_VAL;
    }

    char *save = calloc(strlen(parsedRequest->address) + 1, sizeof(char));
    strcpy(save, parsedRequest->address);
    if (string_occurrence(save, '/') < 2) {
        free(save);
        return INVALID_VAL;
    }
    char **addrSplit = split_by_char(save, '/', 3);
    if (strcmp(addrSplit[0], "")) {
        free(addrSplit);
        return INVALID_VAL;
    }
    if (strcmp(addrSplit[1], DO_VAL) && strcmp(addrSplit[1], DO_INT)) {
        free(addrSplit);
        return INVALID_VAL;
    }
    if (!strcmp(addrSplit[1], DO_VAL)) {
        if (te_validate(addrSplit[2])) {
            free(addrSplit);
            return VALIDATION;
        } else {
            free(addrSplit);
            return INVALID_VAL;
        }
    } else {
        if (string_occurrence(addrSplit[2], '/') < 4) {
            free(addrSplit);
            return INVALID_INT;
        }
        char **intParams = split_by_char(addrSplit[2], '/', 5);
        if (!check_fields_only_nums(intParams)) {
            free(addrSplit);
            free(intParams);
            return INVALID_INT;
        }
        if (te_validate(intParams[4])) {
            free(addrSplit);
            free(intParams);
            return INTEGRATION;
        } else {
            free(addrSplit);
            free(intParams);
            return INVALID_INT;
        }
    }
}

int te_validate(char *expression) {
    double x;
    te_variable vars[] = {{"x", &x}};
    int errPos;
    te_expr *expr = te_compile(expression, vars, 1, &errPos);
    if (expr) {
        te_free(expr);
        return 1;
    } else {
        return 0;
    }
}

int check_fields_only_nums(char **intParams) {
    double lower, upper;
    int segments, threads;
    char err;
    if (sscanf(intParams[0], "%lf%c", &lower, &err) != 1) {
        free(intParams);
        return 0;
    } else if (sscanf(intParams[1], "%lf%c", &upper, &err) != 1) {
        free(intParams);
        return 0;
    } else if (sscanf(intParams[2], "%d%c", &segments, &err) != 1) {
        free(intParams);
        return 0;
    } else if (sscanf(intParams[3], "%d%c", &threads, &err) != 1) {
        free(intParams);
        return 0;
    }
    if (upper <= lower) {
        free(intParams);
        return 0;
    } else if (segments <= 0) {
        free(intParams);
        return 0;
    } else if (threads <= 0) {
        free(intParams);
        return 0;
    } else if (segments % threads != 0) {
        free(intParams);
        return 0;
    }
    return 1;
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

void handle_request(struct HttpRequest *parsedRequest, int clientSocket,
        sem_t *threadLimiter, struct SignalInformation *progStats) {
    #ifdef DEBUG
    fprintf(stderr, "Incoming request:\n---\nMethod: %s\nAddress: %s\n",
            parsedRequest->method, parsedRequest->address);
    int index = 0;
    if (parsedRequest->headers[index]) {
        fprintf(stderr, "Headers:\n");
    }
    while (parsedRequest->headers[index]) {
        fprintf(stderr, " %s: %s\n", parsedRequest->headers[index]->name,
                parsedRequest->headers[index]->value);
        index++;
    }
    fprintf(stderr, "Body: '%s'\n---\n\n", parsedRequest->body);
    #endif
    
    int type = valid_request(parsedRequest);
    if (type == VALIDATION) {
        increment_completed_validations(progStats);
        respond_valid_request(clientSocket);
    } else if (type == INTEGRATION) {
        int verbose = check_verbose(parsedRequest->headers);
        
        integrate_multithreaded(parsedRequest, clientSocket, threadLimiter,
                verbose, progStats);
    } else if (type == INVALID_INT) {
        increment_rejected_integrations(progStats);
        respond_invalid_request(clientSocket);
    } else {
        respond_invalid_request(clientSocket);
    }
}

int check_verbose(HttpHeader **headers) { 
    int index = 0;
    while (headers[index]) {
        if (!strcmp(headers[index]->name, VERBOSE_HEADER_NAME)) {
            if (!strcmp(headers[index]->value, VERBOSE_HEADER_VAL)) {
                return 1;
            }
        }

        index++;
    }
    return 0;
}

void integrate_multithreaded(struct HttpRequest *parsedRequest,
        int clientSocket, sem_t *threadLimiter, int verbose,
        struct SignalInformation *progStats) {
    char **params = split_by_char(parsedRequest->address, '/', 7);
    int segments, threads;
    sscanf(params[4], "%d", &segments);
    sscanf(params[5], "%d", &threads);
    double lower, upper;
    sscanf(params[2], "%lf", &lower);
    sscanf(params[3], "%lf", &upper);
    
    if (segments % threads != 0 || segments > INT_MAX || threads > INT_MAX) {
        respond_invalid_request(clientSocket);
        return;
    }
    
    double threadWidth = (upper - lower) / threads;
    pthread_t *threadIds = malloc(sizeof(pthread_t));
    threadIds[0] = 0;

    for (int i = 0; i < threads; i++) {
        struct IntegrationParams *intParams = malloc(
                sizeof(struct IntegrationParams));

        intParams->lower = (i * threadWidth) + lower;
        intParams->upper = ((i + 1) * threadWidth) + lower;
        intParams->segments = (segments / threads);
        intParams->width = threadWidth;
        intParams->num = i + 1;
        intParams->function = calloc(strlen(params[6]) + 1, sizeof(char));
        strcpy(intParams->function, params[6]);
        intParams->threadLimiter = threadLimiter;
        
        if (threadLimiter) {
            sem_wait(threadLimiter);
        }
        increment_threads_used(progStats);
        pthread_t threadId;        
        pthread_create(&threadId, NULL, computation_thread_handler,
                (void *)intParams);
        for (int i = 0; i >= 0; i++) { // style pinged me without i>=0
            if (threadIds[i] == 0) {
                threadIds = realloc(threadIds, (i + 2) * sizeof(pthread_t));
                threadIds[i] = threadId;
                threadIds[i + 1] = 0;
                break;
            }
        }
    }
    finalise_integration(threadIds, clientSocket, verbose, progStats);
}

void *computation_thread_handler(void *threadArgument) {
    struct IntegrationParams *intParams = (struct IntegrationParams *)
            threadArgument;

    double x;
    te_variable vars[] = {{"x", &x}};
    int errPos;

    te_expr *expr = te_compile(intParams->function, vars, 1, &errPos);

    double intValue = do_math(expr, &x, intParams->lower, intParams->upper,
            intParams->segments);
    te_free(expr);

    struct ThreadResult *intResult = malloc(sizeof(
            struct ThreadResult));

    intResult->num = intParams->num;
    intResult->lower = intParams->lower;
    intResult->upper = intParams->upper;
    intResult->result = intValue;

    // if the threadLimiter doesn't exist, it's null, so check it's non-null
    if (intParams->threadLimiter) {
        sem_post(intParams->threadLimiter);
    }

    #ifdef DEBUG
    fprintf(stderr, "Thread %li result: %lf\n", pthread_self(),
            intResult->result);
    #endif

    return (void *)intResult;
}

double do_math(te_expr *expr, double *x, double lower, double upper,
        int segments) {
    double integrationResult = 0.0;
    for (int i = 0; i <= segments; i++) {
        *x = lower + (i * (upper - lower) / segments);

        double function;
        if (i != 0 && i != segments) {
            function = 2 * te_eval(expr);
        } else {
            function = te_eval(expr);
        }
        integrationResult += function;
    }

    integrationResult *= (upper - lower) / (2 * segments);
    return integrationResult;
}

void finalise_integration(pthread_t *threadIds, int clientSocket,
        int verbose, struct SignalInformation *progStats) {
    char *respVerb = calloc(1, sizeof(char));
    char *verBuffer = malloc(128 * sizeof(char));
    int verLen = 0;
    double result = 0.0;

    // threads are created in order from low to high, so going over creation
    // order finalises them from low to high
    int i = 0;
    while (threadIds[i] != 0) {
        struct ThreadResult *singleResult;

        pthread_join(threadIds[i], (void **)(&singleResult));
       
        if (verbose) {
            memset(verBuffer, '\0', 128);
            sprintf(verBuffer, "thread %i:%lf->%lf:%lf\n", singleResult->num,
                    singleResult->lower, singleResult->upper,
                    singleResult->result);

            verLen += strlen(verBuffer);
            respVerb = realloc(respVerb, (verLen + 1) * sizeof(char));
            strcat(respVerb, verBuffer);
        }
        
        result += singleResult->result;
        free(singleResult);
        i++;
    }

    char *resultStr = calloc(20, sizeof(char));
    sprintf(resultStr, "%lf\n", result);
    
    char *respBody = calloc(strlen(resultStr) + strlen(respVerb) + 20,
            sizeof(char));

    strcat(respBody, respVerb);
    strcat(respBody, resultStr);

    char *response = construct_HTTP_response(200, "OK", NULL, respBody);
    write(clientSocket, response, strlen(response));

    increment_completed_integrations(progStats);
    free(verBuffer);
    free(respVerb);
    free(respBody);
    free(response);
}
