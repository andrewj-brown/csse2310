#include "serverLib.h"
#include "serverHupStruct.h"

#ifndef S_SIGNAL
#define S_SIGNAL

/**
 * Registers the SIGHUP handler for printing output.
 *
 * return (struct SignalInformation *): a struct for storing SIGHUP output info
 *
 * Note that this function is called once, and the resulting return value is
 * used as an argument to every other function listed here.
 */
struct SignalInformation *register_sig_handler(void);

/**
 * Waits for the mutex, then increments the number of active clients.
 */
void increment_active_clients(struct SignalInformation *progStats);

/**
 * Waits for the mutex, then decrements the number of active clients.
 */
void decrement_active_clients(struct SignalInformation *progStats);

/**
 * Waits for the mutex, then increments the number of completed validatsions.
 */
void increment_completed_validations(struct SignalInformation *progStats);

/**
 * Waits for the mutex, then increments the numbre of completed integrations.
 */
void increment_completed_integrations(struct SignalInformation *progStats);

/**
 * Waits for the mutex, then increments the number of rejected integrations
 */
void increment_rejected_integrations(struct SignalInformation *progStats);

/**
 * Waits for the mutex, then increments the number of threads the program used.
 */
void increment_threads_used(struct SignalInformation *progStats);

/**
 * Recieves a SIGHUP and sets the global flag.
 *
 * int s: signal number (irrelevant)
 */
void recieve_sighup(int s);

/**
 * "busy" waits for a sighup to come in, takes the mutex, and prints a bunch of
 * stuff as per the spec. Designed to run in a new thread.
 *
 * void *threadArgument: Really a struct SignalInformation, see above.
 */
void *output_on_sighup(void *threadArgument);

#endif
