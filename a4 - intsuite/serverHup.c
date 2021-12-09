#include "serverLib.h"
#include "serverHup.h"
#include "serverHupStruct.h"

/**
 * Global signal-handling variable.
 */
int recievedSighup;

struct SignalInformation *register_sig_handler(void) {
    struct SignalInformation *progStats = malloc(sizeof(
            struct SignalInformation));

    progStats->numActiveClients = 0;
    progStats->completedValidations = 0;
    progStats->completedIntegrations = 0;
    progStats->rejectedIntegrations = 0;
    progStats->threadsUsed = 0;
    progStats->mutex = malloc(sizeof(sem_t));

    sem_init(progStats->mutex, 0, 1);

    recievedSighup = 0;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = recieve_sighup;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, 0);

    pthread_t threadId;
    pthread_create(&threadId, NULL, output_on_sighup, (void *)progStats);
    pthread_detach(threadId);
    
    return progStats;
}

void recieve_sighup(int s) {
    recievedSighup = 1;
}

void increment_active_clients(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->numActiveClients++;
    sem_post(progStats->mutex);
}

void decrement_active_clients(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->numActiveClients--;
    sem_post(progStats->mutex);
}

void increment_completed_validations(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->completedValidations++;
    sem_post(progStats->mutex);
}

void increment_completed_integrations(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->completedIntegrations++;
    sem_post(progStats->mutex);
}

void increment_rejected_integrations(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->rejectedIntegrations++;
    sem_post(progStats->mutex);
}

void increment_threads_used(struct SignalInformation *progStats) {
    sem_wait(progStats->mutex);
    progStats->threadsUsed++;
    sem_post(progStats->mutex);
}

void *output_on_sighup(void *threadArgument) {
    struct SignalInformation *progStats = (struct SignalInformation *)
            threadArgument;

    while (1) {
        if (recievedSighup) {
            recievedSighup = 0;

            sem_wait(progStats->mutex);

            fprintf(stderr, "Connected clients:%i\n",
                    progStats->numActiveClients);
            fprintf(stderr, "Expressions checked:%i\n",
                    progStats->completedValidations);
            fprintf(stderr, "Completed jobs:%i\n",
                    progStats->completedIntegrations);
            fprintf(stderr, "Bad jobs:%i\n",
                    progStats->rejectedIntegrations);
            fprintf(stderr, "Total threads:%i\n",
                    progStats->threadsUsed);            

            sem_post(progStats->mutex);
            fflush(stderr);
        }
    }

    return NULL;
}
