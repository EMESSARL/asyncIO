#include <netdb.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define BACKLOG_SIZE 10

int resolve_address(struct sockaddr *sa, socklen_t *salen, const char *host, 
  const char *port, int family, int type, int proto){
  struct addrinfo hints, *res;
  int err;
  memset(&hints,0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = proto;
  hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV | AI_PASSIVE;
  if((err = getaddrinfo(host, port, &hints, &res)) !=0 || res == NULL){
     fprintf(stderr, "failed to resolve address :%s:%s\n", host, port);
     return -1;
  }
  memcpy(sa, res->ai_addr, res->ai_addrlen);
  *salen = res->ai_addrlen;
  freeaddrinfo(res);
  return 0;
}

int TCP_Connect(int af, char *server_ipaddr, char *server_port)
{
  int ret, sd;
  struct sockaddr server_addr;
  socklen_t salen;
  if ((sd = socket(af, SOCK_STREAM, 0)) < 0){
    return sd;
  }
  if(resolve_address(&server_addr, &salen, server_ipaddr, server_port, 
      AF_INET, SOCK_STREAM, IPPROTO_TCP)!= 0){
      fprintf(stderr, "Erreur de configuration de sockaddr\n");
      return -1;
  }
  if((ret = connect(sd, &server_addr, salen) ) < 0){
    fprintf(stderr, "Ici haha (%d) (%s:%s)\n", ret, strerror(errno), server_port);
    return ret;
  }
  fprintf(stderr, "con ret %d:%s:%d\n", ret, server_port,sd);
  return sd;
}

int tcp_listen(char *server_ipaddr, char *server_port){
  int sd = 0;
  struct sockaddr server_addr;
  socklen_t salen;
  sd = socket(AF_INET, SOCK_STREAM,0);
  if(sd == -1){
    perror("socket échec !!");
    return -1;
  }
  if(resolve_address(&server_addr, &salen, server_ipaddr, server_port, 
      AF_INET, SOCK_STREAM, IPPROTO_TCP)!= 0){
      perror("Erreur de configuration de sockaddr\n");
      return -1;
  }
  if(bind(sd, &server_addr, salen) < 0){
    perror("Echec de liaison !!");
    return -1;
  }
  listen(sd,BACKLOG_SIZE);
  return sd; 
}
