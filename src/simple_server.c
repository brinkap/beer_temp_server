#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>

#define KGRN "\033[0;32;32m"
#define KCYN "\033[0;36m"
#define KRED "\033[0;32;31m"
#define KYEL "\033[1;33m"
#define KBLU "\033[0;32;34m"
#define KCYN_L "\033[1;36m"
#define KBRN "\033[0;33m"
#define RESET "\033[0m"

static int destroy_flag = 0;
static int connection_flag = 0;
static int writeable_flag = 0;

static void INT_HANDLER(int signo) {
  destroy_flag = 1;
}

struct session_data {
  int fd;
};

static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in) 
{
  if (str == NULL || wsi_in == NULL)
    return -1;

  int n;
  int len;
  unsigned char *out = NULL;

  if (str_size_in < 1) 
    len = strlen(str);
  else
    len = str_size_in;

  out = (unsigned char *)malloc(sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
  //* setup the buffer*/
  memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
  //* write out*/
  n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

  printf(KBLU"[websocket_write_back] %s\n"RESET, str);
  //* free the buffer*/
  free(out);

  return n;
}


static int ws_service_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason, void *user,
    void *in, size_t len)
{

  switch (reason) {

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf(KYEL"[Main Service] Connect with server success.\n"RESET);
      connection_flag = 1;
      websocket_write_back(wsi, (char*)"CLT - Client connected to server!", -1);
      break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      printf(KRED"[Main Service] Connect with server error.\n"RESET);
      destroy_flag = 1;
      connection_flag = 0;
      break;

    case LWS_CALLBACK_CLOSED:
      printf(KYEL"[Main Service] LWS_CALLBACK_CLOSED\n"RESET);
      destroy_flag = 1;
      connection_flag = 0;
      break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
      printf(KCYN_L"[Main Service] Client received:%s\n"RESET, (char *)in);

      break;
    case LWS_CALLBACK_CLIENT_WRITEABLE :
      printf(KYEL"[Main Service] On writeable is called. send byebye message\n"RESET);
      websocket_write_back(wsi, (char*)"Byebye! See you later", -1);
      writeable_flag = 1;
      break;

    default:
      break;
  }

  return 0;
}


int main(void)
{
  //* register the signal SIGINT handler */
  struct sigaction act;
  act.sa_handler = INT_HANDLER;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction( SIGINT, &act, 0);

  struct lws_context *context = NULL;
  struct lws_context_creation_info info;
  struct lws *wsi = NULL;
  struct lws_protocols protocol;

  memset(&info, 0, sizeof info);
  //info.port = CONTEXT_PORT_NO_LISTEN;
  info.port = 7681;
  info.iface = NULL;
  info.protocols = &protocol;
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.extensions = NULL;
  info.gid = -1;
  info.uid = -1;
  info.options = 0;

  protocol.name  = "my-echo-protocol";
  protocol.callback = &ws_service_callback;
  protocol.per_session_data_size = sizeof(struct session_data);
  protocol.rx_buffer_size = 0;
  protocol.id = 0;
  protocol.user = NULL;

  context = lws_create_context(&info);
  printf(KRED"[Main] context created.\n"RESET);

  if (context == NULL) {
    printf(KRED"[Main] context is NULL.\n"RESET);
    return -1;
  }
  /*
  wsi = lws_client_connect(context, "localhost", 8124, 0,
      "/", "localhost:8124", NULL,
      protocol.name, -1);
  if (wsi == NULL) {
    printf(KRED"[Main] wsi create error.\n"RESET);
    return -1;
  }

  printf(KGRN"[Main] wsi create success.\n"RESET);
  */

  while(!destroy_flag)
  {
    lws_service(context, 50);
  }

  lws_context_destroy(context);

  return 0;
}
