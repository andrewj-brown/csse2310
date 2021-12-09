#include "serverLib.h"

#ifndef S_SIGHUP_STRUCT
#define S_SIGHUP_STRUCT

/**
 * Struct for storing all the information to be output on SIGHUP
 */
struct SignalInformation {
    int numActiveClients;
    int completedValidations;
    int completedIntegrations;
    int rejectedIntegrations;
    int threadsUsed;
    sem_t *mutex;
};

#endif
