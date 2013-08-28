#ifndef WEBSERVER_PROCESS_H
#define WEBSERVER_PROCESS_H

#include "webserver_config.h"
#include "http_request.h"
#include "http_response.h"
#include "sockets.h"
#include <stdio.h>

void webserver_process_request(HttpRequest*     request,
                               ClientSocket*    client,
                               WebServerConfig* config);

void webserver_process_get   (HttpRequest* request, ClientSocket* client);
void webserver_process_head  (HttpRequest* request, ClientSocket* client);
void webserver_process_post  (HttpRequest* request, ClientSocket* client);
void webserver_process_put   (HttpRequest* request, ClientSocket* client);
void webserver_process_delete(HttpRequest* request, ClientSocket* client);
void webserver_process_error (HttpRequest* request, ClientSocket* client);
void webserver_echo_request  (HttpRequest* request, ClientSocket* client);

void webserver_send_response(ClientSocket*    client,
                             enum EHttpStatus status,
                             const char*      body,
                             const char*      content_type);

#endif
