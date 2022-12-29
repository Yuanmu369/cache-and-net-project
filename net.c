#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
    len = recv(fd, buf, len, 0);
    //buf[len] = '\0';
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
    if( send(fd, buf, len, 0) < 0){
        printf("send msg error\n");
        return false;
    }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
    uint8_t buf[264] = {0};
    uint16_t len;
    //printf("###%s, %d\n", __func__, __LINE__);
    nread(fd, 264, buf);
    memcpy(&len, &buf[0], 2);
    memcpy(op, &buf[2], 4);
    memcpy(ret, &buf[6], 2);
    if(block){
        memcpy(block, &buf[8], 256);
    }
    //printf("###%s, %d\n", __func__, __LINE__);
    len = ntohs(len);
    *op = ntohl(*op);
    //printf("####op:%x, ret=%d\n", *op, *ret);
    return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
    uint8_t buf[264] = {0};
    uint16_t len = 264;
    bool ret;
    uint8_t command = (op>>26)&0x3F;
    uint16_t big_len = htons(len);
    uint32_t big_op = htonl(op);
    if(command == JBOD_WRITE_BLOCK){
        memcpy(&buf[0], &big_len, 2);
        memcpy(&buf[2], &big_op, 4);
        memcpy(&buf[8], block, 256); 
    }else{
        memcpy(&buf[0], &big_len, 2);
        memcpy(&buf[2], &big_op, 4);
    }
    ret = nwrite(sd, 264, buf);
    //printf("####op:%x, len=%d\n", op, len);
    return ret;
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
    struct sockaddr_in  servaddr;
    if( (cli_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error\n");
        return false;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n",ip);
        return false;
    }

    if( connect(cli_sd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("connect error\n");
        return false;
    }
    return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
    close(cli_sd);
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
    uint16_t ret = 0;
    if(!send_packet(cli_sd, op, block)){
        return 1;
    }
    if(!recv_packet(cli_sd, &op, &ret, block)){
        return 1;
    }
    return 0;
}
