#include "serverLib.h"

#ifndef S_HTTP_STRUCT
#define S_HTTP_STRUCT

/**
 * A parsed HttpRequest with all fields from parse_HTTP_request().
 */
struct HttpRequest {
    char *method;
    char *address;
    HttpHeader **headers;
    char *body;
};

#endif
