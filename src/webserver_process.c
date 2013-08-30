#include "webserver_process.h"

//##############################################################################
// Webserver request-handling functions
//   TODO - reuse shared memory location for writing responses
//##############################################################################

void webserver_process_request(HttpRequest*     request,
                               ClientSocket*    client,
                               WebServerConfig* config)
{
  // If in echo mode, echo the request info back to the user
  if (config->echo) {
    webserver_echo_request(request, client);
  } else {
  // Otherwise, respond normally
    switch (request->method) {
      case HTTP_METHOD_GET:  webserver_process_get  (request, client); break;
      case HTTP_METHOD_HEAD: webserver_process_head (request, client); break;
      case HTTP_METHOD_POST: webserver_process_post (request, client); break;
      case HTTP_METHOD_PUT:  webserver_process_put  (request, client); break;
      default:               webserver_process_error(request, client); break;
    }
  }
}

void webserver_process_get(HttpRequest* request, ClientSocket* client)
{
  // Look up resource and return it
  // - If found, return 200 / OK
  // - If not, return 404 / Not Found

  // Append request URI to FILE_STORAGE_ROOT
  // If file exists, return it with 200 / OK
  // Else file is missing so return 404 / Not Found
  

  // TODO: 
  // - Refactor file handling into another file
  FILE    *file;
  char    *buffer;
  long    numbytes;

  file = fopen("public/index.html", "r");

  if (file != NULL) {
    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    buffer = (char *)calloc(numbytes, sizeof(char));
    
    if (buffer != NULL) {
      fread(buffer, sizeof(char), numbytes, file);
      const char* buff = buffer;

      webserver_send_response(client, HTTP_STATUS_OK, buff, "text/html");

      fclose(file);
      free(buffer);
    }
  } else {
    const char* body = "<html><body>Not Found!</body></html>";
    webserver_send_response(client, HTTP_STATUS_NOT_FOUND, body, "text/html");
  }

}

void webserver_process_head(HttpRequest* request, ClientSocket* client)
{
  // Look up resource and return meta-info via headers
  // - Should be identical to meta-info returned from GET; just w/o a body
  webserver_send_response(client, HTTP_STATUS_OK, 0, 0);
}

void webserver_process_post(HttpRequest* request, ClientSocket* client)
{
  // Respond with 501 / Not Implemented
  webserver_send_response(client, HTTP_STATUS_NOT_IMPLEMENTED, 0, 0);
  // NOTES:
  // Get content length
  // - If missing, send 411 / Length Required
  // - If different than message body, send 400 / Bad Request
}

void webserver_process_put(HttpRequest* request, ClientSocket* client)
{
  // Respond with 501 / Not Implemented
  webserver_send_response(client, HTTP_STATUS_NOT_IMPLEMENTED, 0, 0);
}

void webserver_process_delete(HttpRequest* request, ClientSocket* client)
{
  // Respond with 404 / Not Found
  webserver_send_response(client, HTTP_STATUS_NOT_FOUND, 0, 0);
}

void webserver_process_error(HttpRequest* request, ClientSocket* client)
{
  // Respond with 501 / Not Implemented
  webserver_send_response(client, HTTP_STATUS_NOT_IMPLEMENTED, 0, 0);
}

void webserver_echo_request(HttpRequest* request, ClientSocket* client)
{
  // Just send back the full HTTP request from the client
  webserver_send_response(client, HTTP_STATUS_OK, client->data, 0);
}

void webserver_send_response(ClientSocket*    client,
                             enum EHttpStatus status,
                             const char*      body,
                             const char*      content_type)
{
  // Build the Content-Length string
  char content_length[20] = {0};
  snprintf(content_length, 20, "%zu", (body ? strlen(body) : 0));
  if (!content_type) { content_type = "text/plain"; }

  // Build the response object
  HttpResponse* res = http_response_new();
  http_response_set_status(res, HTTP_VERSION_1_0, status);
  http_response_add_header(res, "Server", "webserver");
  http_response_add_header(res, "Content-Length", content_length);
  if (body) { http_response_add_header(res, "Content-Type", content_type); }
  if (body) { http_response_set_body(res, body); }

  // Send the response and clean up
  client_socket_send(client, http_response_string(res), http_response_length(res));
  http_response_free(res);
}
