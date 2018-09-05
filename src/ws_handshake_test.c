#include <stdio.h>
#include <string.h>
#include "base64.h"
#include <openssl/sha.h>

int main(){

//  char key[]="dGhlIHNhbXBsZSBub25jZQ==";
  char key[]="4FqwP+zUH6LvCW9b54EQjA==";
  char magic_str[]="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  char accept_key[1024];
  strcpy(accept_key, key);
  strcat(accept_key, magic_str);
  printf("accept key is %s\n", accept_key);
  char digest[SHA_DIGEST_LENGTH];
  SHA1((unsigned char*)accept_key, strlen(accept_key), (unsigned char*) digest);

    char msg[1024] = "\
HTTP/1.1 101 Switching Protocols\r\n \
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Accept: ";
    size_t outlen;
    char *base64_encoded = base64_encode(digest, strlen(digest), &outlen);
    base64_encoded[outlen] = '\0'; // likely uneeded
    strcat(msg,base64_encoded);
    strcat(msg, "\r\n");
    printf("%s", msg);
    printf("Accept key should be s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\n");
    return 0;
}


