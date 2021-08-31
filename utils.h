#ifndef __UTILS_H
#define __UTILS_H
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
int resolve_address(struct sockaddr *sa, socklen_t *salen, const char *host, 
  const char *port, int family, int type, int proto);

int TCP_Connect(int af, char *server_ipaddr, char *server_port);
int tcp_listen(char *server_ipaddr, char *server_port);
#endif /* __UTILS_H */
