#include "webserver.h"
#include "http_request.h"
#include "http_response.h"
#include "sockets.h"
#include "logging.h"
#include "utils.h"
#include "webserver_process.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

//##############################################################################
// Constants and utilities
//##############################################################################
// Max number of incoming connections that maybe queued by the server's socket
static const int MAX_PENDING_CONNS = 100;

// Path to the files to server // TODO - make this configurable!
// static const char* FILE_STORAGE_ROOT = "/Users/ericraio/dev/webserver/sites";

// Check status. On error, print message and return from calling function.
#define return_on_error(status, errmsg) \
if (!status.ok) { \
  log_err("%s (errno: %i)", errmsg, status.errnum); \
  return; \
}

//##############################################################################
// Signal handling
//##############################################################################
// Objects for the signal handler to manipulate
static bool keep_running = true;
static ClientSocket* curr_client_socket = NULL;
static ServerSocket* curr_server_socket = NULL;

// Upon receipt of a signal, close sockets and have the webserver stop running
void handle_signal(int sig)
{
  keep_running = false;
  log_all("Receive signal %i. Shutting down.", sig);
  if (curr_client_socket) { client_socket_close(curr_client_socket); }
  if (curr_server_socket) { server_socket_close(curr_server_socket); }
}

//##############################################################################
// Webserver request-handling and response functions
//##############################################################################
// This function routes the request to the proper function below
void webserver_process_request(HttpRequest* request,
                               ClientSocket* client,
                               WebServerConfig* config);
// Handle different HTTP methods, or a bad request
void webserver_process_get    (HttpRequest* request, ClientSocket* client);
void webserver_process_head   (HttpRequest* request, ClientSocket* client);
void webserver_process_post   (HttpRequest* request, ClientSocket* client);
void webserver_process_put    (HttpRequest* request, ClientSocket* client);
void webserver_process_delete (HttpRequest* request, ClientSocket* client);
void webserver_process_error  (HttpRequest* request, ClientSocket* client);

// Echo the request back to the client user for development/debugging.
void webserver_echo_request   (HttpRequest* request, ClientSocket* client);

// Send an HTTP response
// - Use NULL to indicate no body
// - If content_type is NULL, use "text/plain"
void webserver_send_response(ClientSocket*       client,
                                     enum        EHttpStatus status,
                                     const char* body,
                                     const char* content_type);

//##############################################################################
// Webserver
//##############################################################################
void webserver_start(WebServerConfig* config) {
  const int port = config->port;
  log_all("Initializing server on port %i", port);

  // Set up signal handler
  signal(SIGINT,  handle_signal);
  signal(SIGKILL, handle_signal);
  signal(SIGTERM, handle_signal);

  // Make sure we can open the log files
  echo_log_to_console(true);
  if (!open_log_files().ok) { return; }

  // Create and instantiate the socket
  Status status;
  ServerSocket server;

  // Set the current server socket so the signal handler can close the socket
  curr_server_socket = &server;

  // Initialize the server socket and start listening for incoming connections
  status = server_socket_init(&server);
  return_on_error(status, "Error initializing server socket");

  status = server_socket_bind(&server, port);
  return_on_error(status, "Error binding socket to port");

  status = server_socket_listen(&server, MAX_PENDING_CONNS);
  return_on_error(status, "Error having socket listen for incoming connections");

  log_std("Server now listening for incoming connections on port %i", port);

  // Accept incoming connections and service their requests
  while(keep_running) {
    // Create the client socket save it s the signal handler can close it
    ClientSocket client;
    client_socket_init(&client);
    curr_client_socket = &client;

    // Accept the next connection. On failure, try again.
    // - If we're shutting down, skip printing the error
    status = server_socket_accept(&server, &client);
    if (!status.ok) {
      if (keep_running) { log_err("Error acception incoming connection (errno: %i)", status.errnum); }
      continue;
    }

    // Where is the connection coming from?
    const char* ip = client_socket_get_ip(&client);
    const int port = client_socket_get_port(&client);

    // Read the incoming data
    status = client_socket_recv(&client);
    if (!status.ok) {
      log_err("%s:%i | Error reading data from client (errno: %i)", ip, port, status.errnum);
      client_socket_close(&client);
      continue;
    }

    // Check if data receive. If not, log an error
    if (!client.data) {
      log_err("%s:%i | Got no data from client", ip, port);
      client_socket_close(&client);
      continue;
    }

    // Print the entire request if verbose mode enabled
    if (config->verbose) {
      printf("------------- received ---------------\n");
      printf("%s\n", client.data);
      printf("--------------------------------------\n");
    }

    // Parse the request
    HttpRequest request;
    http_request_init(&request);
    bool status = http_request_parse(&request, client.data);

    // If parsing succeeded, log a message and process the response
    if (status) {
      const char* method = http_method_to_string(request.method);
      const char* version = http_version_to_string(request.version);
      log_std("%s:%i | %s %s %s", ip, port, method, request.uri, version);
      webserver_process_request(&request, &client, config);
    } else {
      // If parsing failed, log the error and respond with a 400 / Bad Request
      log_err("%s:%i | %s", ip, port, request.error);
      webserver_send_response(&client, HTTP_STATUS_BAD_REQUEST, request.error, 0);
    }

    // Clean up: request-handling resources
    curr_client_socket = NULL;
    http_request_free(&request);
    client_socket_close(&client);
  }

  // Clean up: webserver resources
  curr_server_socket = NULL;
  server_socket_close(&server);
  close_log_files();
}
