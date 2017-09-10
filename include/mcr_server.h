#ifndef _MCR_SERVER_H
#define _MCR_SERVER_H

int run_serverloop(int server_sock, struct server_config config);
int open_serversock(const char *port, int backlog);
void close_serversock();
#endif
