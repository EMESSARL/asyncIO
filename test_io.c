#define _GNU_SOURCE    
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/epoll.h>

#include <sys/types.h>  
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h> 
#include <sys/socket.h>
#include "utils.h"
#include "test_io.h"

#define MAX_HOST_NAME_SIZE 255

#define PORT 8080
#define MAX_PORT_SIZE 6
#define MAX_NAME_SIZE 255
#define BUFFSIZE 65536
//#define false 0;
#define MAX_EVENTS 5

void handle_alarm(int number){
  fprintf(stderr, "\n%ld a recu le signal %d (%s)\n", 
    (long) getpid(), number, strsignal(number));	        
}




int main(int argc, char *argv[])
{

   int pfd[2]; //pour le pipe
   pid_t pid;
   fd_set readset, writeset;
   int engine = 0;
   char buff[BUFFSIZE];
   char buffer[BUFFSIZE];
   int ret, ret_sel,fd_max, timeout = 0, bytestosend = 0 ;
   int offset = 0, bytesrecved = 0 ;
   int should_write = 0,i;
   int sock,data, data_sd, rt;
   sigset_t pselect_set;
   sigset_t ppoll_set;
   struct timeval out_time;
   struct timespec out_time1,out_time2;
   struct epoll_event event1, event2, events[MAX_EVENTS];
   socklen_t len;
  struct sockaddr  client, client2;
   
   char host[MAX_NAME_SIZE];
   char c;
   char port[MAX_PORT_SIZE];
   bool with_server = false;
   bool with_client= false;
   out_time.tv_sec = 3;
   out_time1.tv_sec=5;
   out_time.tv_usec = 0;
   out_time1.tv_nsec=10;
   out_time2.tv_sec=10;
   out_time2.tv_nsec=10;
   struct pollfd pfds[2];
   fcntl(0, F_SETFL, O_NONBLOCK );
   fcntl(1, F_SETFL, O_NONBLOCK );
   
   FD_ZERO(&readset);
   FD_ZERO(&writeset);
   memset(buffer, 0, BUFFSIZE); 
   sigemptyset(&pselect_set);
   sigemptyset(&ppoll_set);
   int epoll_fd, event_count;
    
   
   
   while ((c = getopt(argc, argv, "tpolescP:h:")) != -1){
      
      switch (c){
         case 't':
            engine = SELECT_WITH_TIME_ENGINE;
            break;
         case 'p':
            engine = PSELECT_ENGINE;
            break;
         case 'o':
            engine = POLL_ENGINE;
            break;
         case 'l':
            engine = PPOLL_ENGINE;
            break;
         case 'e':
            engine = EPOLL_ENGINE;
            break;
         case 's':
            with_server =true;
            break;
         case 'c':
             with_client =true;
            break;
         case 'P':
	     strncpy(port, optarg, MAX_PORT_SIZE);
	    break;
	 case 'h':
	     strncpy(host, optarg, MAX_NAME_SIZE);
	    break;
	 default:
            engine = NO_ENGINE;
      }
   }
   
    
       
    if(with_client){
          data_sd =  TCP_Connect(AF_INET, host, port);
       if(data_sd < 0 ){
          fprintf(stderr, "Echec de la tentative de connection au serveur\n");
          fprintf(stderr, "%s\n", strerror(errno));
          return 1;
       }
       printf("connection au serveur réussie\n");
       fd_max =data_sd;
       fcntl(data_sd, F_SETFL, O_NONBLOCK );
       FD_SET(data_sd, &readset);
       FD_SET(data_sd, &writeset);
      }
     
      if(with_server){	       
	   sock = tcp_listen(host, port);
	   if(sock  < 0){
	      printf("failed to listen");
	      return -1;
	   }
	   fcntl(sock, F_SETFL, O_NONBLOCK );
	   
	   
     }
   
   if ((pipe(pfd) == -1)){
      fprintf(stderr, "Erreur lors de la création du pipe\n");
      return 1;
   }
   
   fcntl(pfd[0], F_SETFL, O_NONBLOCK);
   fcntl(pfd[1], F_SETFL, O_NONBLOCK);

   /*Créer un processus fils pour lire dans notre pipe*/
   if ((pid = fork()) == -1){
      fprintf(stderr, "Echec de la création du processus fils\n");
      return 1;
   }

   if (pid == 0){
      long bytesrecv = 0;
      long byteswrite = 0;
      fd_max = 0;
      pid_t ppid = getppid();
      close(pfd[1]); //fermer l'extrémité de lecture pour éviter des comportements bizarres
      
      while (1){
      
         switch(engine){
     	   case SELECT_WITH_TIME_ENGINE:
     	  // case PSELECT_ENGINE:
     	   case PSELECT_ENGINE:
              if(with_client){
                 fd_max = data_sd ;
     	         FD_SET(data_sd, &readset);
                 FD_SET(data_sd, &writeset);
              }     
     	      FD_SET(1, &writeset);
              fd_max = 1;
              FD_SET(pfd[0], &readset);
              if (pfd[0] > fd_max)
                  fd_max = pfd[0];
              break;
           case POLL_ENGINE:
           case PPOLL_ENGINE:
             pfds[0]=(struct pollfd){
                 .fd=1,
                 .events = POLLOUT| POLLERR,
                 .revents = 0
              };
              
              pfds[1]=(struct pollfd){
                 .fd=pfd[0],
                 .events =POLLIN | POLLERR,
                 .revents = 0
              };
              break;
           case EPOLL_ENGINE:
	         
		  epoll_fd = epoll_create1(0);
		 
		  if(epoll_fd == -1)
		  {
		    fprintf(stderr, "erreur de creation du descripteur de fichier pour epoll\n");
		    return 1;
		  }
		 
		  event1=(struct epoll_event){
		     .events = EPOLLIN,
		     .data.fd = pfd[0],
		  };
		  event2=(struct epoll_event){
		     .events = EPOLLOUT,
		     .data.fd = 1,
		  }; 
		 
		  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfd[0], &event1))
		  {
		    fprintf(stderr, " Impossible d'ajouter  pfd[0]  au descipteur de fichier de epoll\n");
		    close(epoll_fd);
		    return 1;
		  }
		  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD,1, &event2))
		  {
		    fprintf(stderr, "Impossible d'ajouter 1  au descipteur de fichier de epoll\n");
		    close(epoll_fd);
		    return 1;
		  }	 
		
		  break;
           default:
               
              break;
     	}
         
        switch(engine){
           case SELECT_WITH_TIME_ENGINE:
                ret_sel = select(fd_max+1, &readset, &writeset, NULL, NULL);
                break;
          // case 1:
                ret_sel = select(fd_max+1, &readset, &writeset, NULL, &out_time);
                break;
           case PSELECT_ENGINE:
                sigaddset(&pselect_set, SIGALRM);
                ret_sel = pselect(fd_max+1, &readset, &writeset, NULL, &out_time1,&pselect_set );
                break;
           case POLL_ENGINE:
             ret_sel =poll(pfds,2,0);
               break;
           case PPOLL_ENGINE:
           	sigaddset(&ppoll_set, SIGALRM);
                ret_sel = ppoll(pfds,2,&out_time2,&ppoll_set);
                break;
           case EPOLL_ENGINE:        
               event_count= epoll_wait(epoll_fd, events,MAX_EVENTS,3000);
               break;
           default:
               break;
        }
         switch(engine){
           case SELECT_WITH_TIME_ENGINE:       
           //case 1:      
           case PSELECT_ENGINE:
                if (ret_sel < 0){
		    fprintf(stderr, "Erreur du select dans le fils\n");
		    return 1;
		 }
		 else if(ret_sel == 0){
		    fprintf(stderr, "Je suius fatigué d'attendre dans le fils bytesrecv (%ld) byteswrite(%ld)\n", bytesrecv, byteswrite);
		    timeout++;
		    if(timeout >= 3)
		        break;
		    sleep(10);
		 }
		 else
		 {
		    
		    timeout = 0;
		    if (FD_ISSET(pfd[0], &readset)){
		       if(!bytesrecved){
		          bytesrecved = read(pfd[0],buffer, BUFFSIZE);
		          if(engine==2)
		             kill(ppid, SIGALRM);
		          if(bytesrecved < 0){
		             fprintf(stderr, "On a un petit problème lors du read dans le fils\n");
		          }
		          else if (bytesrecved == 0){
		             close(pfd[0]);
		             fprintf(stderr, "end of child\n");
		             break;
		          }
		          else{
		             bytesrecv +=bytesrecved;
		             should_write = 1;
		             offset = 0;
		          }
		       }
		       //memset(buffer, 0, BUFFSIZE);
		    }
		    if(FD_ISSET(1, &writeset)){
		       if(should_write){
		          int m = write(1, buffer+offset, bytesrecved);
		          if(m < 0)
		             fprintf(stderr, "un problème dans le fils %s", strerror(errno));
		          else {  
		             byteswrite +=m;
		             offset +=m;
		             bytesrecved -= m;
		             if(!bytesrecved){
		                should_write = 0;
		                offset = 0;
		             } 
		          }
		          
		       }
		    }                
		 }
		 FD_CLR(pfd[0], &readset);
		 FD_CLR(1, &writeset);  
                break;
           case POLL_ENGINE:
           case PPOLL_ENGINE:
               if (ret_sel < 0){
                    fprintf(stderr,"Une erreur est survenue! dans le poll du fils\n");
                    break;
               }
               else if (ret_sel == 0){
                     fprintf(stderr,"Temps expiré, rien à lire, rien à écrire dans le fils!(avec poll)\n");
               break;
               }
               else{
                if(pfds[1].revents & POLLIN){
		     if(!bytesrecved){
		          bytesrecved = read(pfd[0],buffer, BUFFSIZE);
		          if(engine==4)
		             kill(ppid, SIGALRM);
		          if(bytesrecved < 0){
		             fprintf(stderr, "On a un petit problème lors du read dans le fils(cas de poll)\n");
		          }
		          else if (bytesrecved == 0){
		             close(pfd[0]);
		             fprintf(stderr, "end of child\n");
		             break;
		          }
		          else{
		             bytesrecv +=bytesrecved;
		             should_write = 1;
		             offset = 0;
		               
		          }
		       }  
                }
		 if(pfds[0].events & POLLOUT){
		 	if(should_write){
		          int m = write(1, buffer+offset, bytesrecved);
		          if(m < 0)
		             fprintf(stderr, "un problème dans le fils %s", strerror(errno));
		          else {  
		             byteswrite +=m;
		             offset +=m;
		             bytesrecved -= m;
		             if(!bytesrecved){
		                should_write = 0;
		                offset = 0;
		            
		             } 
		          }
		          
		       }  
            
               }
                
             }
               break;
           case EPOLL_ENGINE:
           if ( event_count < 0){
                    fprintf(stderr,"Une erreur est survenue! dans le poll du fils\n");
                    return 1;
               }
               
               
            else  if ( event_count == -1) {
                   printf("epoll_wait");
                   exit(EXIT_FAILURE);
                   return 1;
              }
             else{
              for(int i = 0; i < event_count; i++){
                 if (events[i].events & EPOLLIN){
                   if(!bytesrecved){
		          bytesrecved = read(pfd[0],buffer, BUFFSIZE);
		          if(engine==4)
		             kill(ppid, SIGALRM);
		          if(bytesrecved < 0){
		             fprintf(stderr, "On a un petit problème lors du read dans le fils(cas de poll)\n");
		          }
		          else if (bytesrecved == 0){
		             close(pfd[0]);
		             fprintf(stderr, "end of child\n");
		             break;
		          }
		          else{
		             bytesrecv +=bytesrecved;
		             should_write = 1;
		             offset = 0;
		               
		          }
		       }  
                 }
                 else if(events[i].events & EPOLLOUT){
                 
                   if(should_write){
		          int m = write(1, buffer+offset, bytesrecved);
		          if(m < 0)
		             fprintf(stderr, "un problème dans le fils %s", strerror(errno));
		          else {  
		             byteswrite +=m;
		             offset +=m;
		             bytesrecved -= m;
		             if(!bytesrecved){
		                should_write = 0;
		                offset = 0;
		            
		             } 
		          }
		          
		       }  
                 }
                
              }
              }
              if(close(epoll_fd))
	       {
		    fprintf(stderr, "Impossible de fermer le descripteur de fichier epoll\n");
		    return 1;
	        }
		 
		 return 1;
              break;
           default:
               break;
        
        } 
      }//Fin while du select dans le fils
      close(pfd[0]);
      exit(0);
   }//FIn du if (si on est dans le fils)

   else
  {
     long bytessent = 0;
     long bytesread = 0;
     if(engine=PSELECT_ENGINE)
        if (signal(SIGALRM, handle_alarm) == SIG_ERR)
          fprintf(stderr, "Signal %d non capture\n", SIGALRM);
     if( engine=PPOLL_ENGINE)
        if (signal(SIGALRM, handle_alarm) == SIG_ERR)
          fprintf(stderr, "Signal %d non capture\n", SIGALRM);
          
      
          
     

     while(1){
     
     
     
      
     	switch(engine){
     	   case SELECT_WITH_TIME_ENGINE:
           case PSELECT_ENGINE:
             if(with_server){
     	         fd_max = sock ;
     	         FD_SET(sock, &readset);
     	          printf("connection au client failed\n");
              }
             FD_SET(0, &readset);
             FD_SET(pfd[1], &writeset);
             if (pfd[1] > fd_max)
                fd_max = pfd[1];
             break;
           case POLL_ENGINE:
           case PPOLL_ENGINE:
              pfds[0]=(struct pollfd){
                 .fd=0,
                 .events = POLLIN | POLLERR,
                 .revents = 0
              };
              
              pfds[1]=(struct pollfd){
                 .fd=pfd[1],
                 .events =POLLOUT ,
                 .revents = 0
              };
              
              break;
           case EPOLL_ENGINE:
                epoll_fd = epoll_create1(0);
		 
		  if(epoll_fd == -1)
		  {
		    fprintf(stderr, "erreur de creation du descripteur de fichier pour epoll\n");
		    return 1;
		  }
		 
		  event1=(struct epoll_event){
		     .events = EPOLLIN,
		     .data.fd = 0,
		  };
		  event2=(struct epoll_event){
		     .events = EPOLLOUT,
		     .data.fd = pfd[1],
		  }; 
		 
		  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event1))
		  {
		    fprintf(stderr, "Impossible d'ajouter pfd[1] au descipteur de fichier de epoll %s\n", strerror(errno));
		    close(epoll_fd);
		    return 1;
		  }
		  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD,pfd[1], &event2))
		  {
		    fprintf(stderr, " Impossible d'ajouter 0  au descipteur de fichier de epoll\n");
		    close(epoll_fd);
		    return 1;
		  }
		break;
         
	  default:
		break;
	    }
         
        switch(engine){
           case SELECT_WITH_TIME_ENGINE:
                   
                ret_sel = select(fd_max+1, &readset, &writeset, NULL, NULL);
      
                break;
           case PSELECT_ENGINE:
                sigaddset(&pselect_set, SIGALRM);
                ret_sel = pselect(fd_max+1, &readset, &writeset, NULL, NULL, &pselect_set);
                break;
           case POLL_ENGINE:
           	
               ret_sel =poll(pfds,2,0);
               break;
          case PPOLL_ENGINE:
             sigaddset(&ppoll_set, SIGALRM);
             ret_sel = ppoll(pfds,2,&out_time2,&ppoll_set);
             break;
           case EPOLL_ENGINE:
              
               event_count= epoll_wait(epoll_fd, events,MAX_EVENTS,1);
               
               break;
           
         
           default:
               break;
        }
        
        switch(engine){
           case SELECT_WITH_TIME_ENGINE:
           case PSELECT_ENGINE:
                 if (ret_sel < 0){
                    fprintf(stderr,"Une erreur est survenue! dans le select du père\n");
                 break;
                 }
                 else if (ret_sel == 0){
                     fprintf(stderr,"Temps expiré, rien à lire, rien à écrire dans le père!\n");
                 break;
                 }
		 else{
		 
                   if(FD_ISSET(sock, &readset)){
                       printf("Try to accept connexion\n");
    	      	       data= accept(sock, (struct sockaddr*)&client, &len);
    		       FD_CLR(sock, &readset);
    		    
    		       if(data>0)
                         printf("connection au client reussie\n");
                       
                      printf("connection au client failed\n");
                      
                  }
                   
                  printf("connection au client failed\n");
		   if(FD_ISSET(0, &readset)){
		      if(!bytestosend){
		         ret = read(0, buffer, BUFFSIZE);
		         if(ret < 0)
		            fprintf(stderr, "Il ya eu un problème lors de la lecture\n");
		         else if(ret>0){
		            should_write = 1;
		            bytestosend += ret;
		            bytesread +=ret;
		         }
		         else {
		      //    fprintf(stderr, "end of father\n");
		          break;
		         }
		      }
		      else{
		         if(offset)
		             fprintf(stderr, "Peut on comblé le vide (%d)(%d)\n", bytestosend, offset);
		      }    
                   }

		    if (FD_ISSET(pfd[1], &writeset))
		    {
		       
		       //scanf("%s", buffer);
		       if (should_write){
		          ret = write(pfd[1], buffer+offset, bytestosend);
		          if(ret < 0){
		             fprintf(stderr, "il ya eu un problème lors de l'écriture %d\n", offset);
		             //break;
		          }
		          bytessent += ret;
		          bytestosend -= ret; 
		          offset +=  ret;
		          
		          if(bytestosend){
		             fprintf(stderr, "on a pas pu tout envoyer %d\n", bytestosend);
		          }else{
		            should_write = 0;
		            offset = 0;
		            //fprintf(stderr, "on a tout envoyé %d\n", bytestosend);
		          //memset(buffer, 0, BUFFSIZE); 
		          }              
		       }
		    }       
		 }
		 
		 FD_CLR(0, &readset);
		 FD_CLR(pfd[1], &writeset);
                break;
          
           case POLL_ENGINE:
           case PPOLL_ENGINE:
                if (ret_sel < 0){
                    fprintf(stderr,"Une erreur est survenue! dans le poll du père\n");
                    break;
               }
               else if (ret_sel == 0){
                     fprintf(stderr,"Temps expiré, rien à lire, rien à écrire dans le père!(avec poll)\n");
               break;
               }
               else{
		if(pfds[0].revents &  POLLIN){
		        if(!bytestosend){
		         ret = read(0, buffer, BUFFSIZE);
		         if(ret < 0)
		            fprintf(stderr, "Il ya eu un problème lors de la lecture\n");
		         else if(ret>0){
		            should_write = 1;
		            bytestosend += ret;
		            bytesread +=ret;
		            //write(1, buffer, BUFFSIZE);
		         }
		         else {
		          fprintf(stderr, "end of father\n");
		          break;
		         }
		      }
		      else{
		         if(offset)
		             fprintf(stderr, "Peut on comblé le vide (%d)(%d)\n", bytestosend, offset);
		      }    
		       //write(1, buffer, BUFFSIZE);
		}
		if(pfds[1].revents & POLLOUT){
                   if (should_write){
		          ret = write(pfd[1], buffer+offset, bytestosend);
		          if(ret < 0){
		             fprintf(stderr, "il ya eu un problème lors de l'écriture %d\n", offset);
		             //break;
		          }
		          bytessent += ret;
		          bytestosend -= ret; 
		          offset +=  ret;
		          
		          if(bytestosend){
		             fprintf(stderr, "on a pas pu tout envoyer %d\n", bytestosend);
		          }else{
		            should_write = 0;
		            offset = 0;
		           // fprintf(stderr, "on a tout envoyé %d\n", bytestosend);
		          //memset(buffer, 0, BUFFSIZE); 
		          }              
		       }
		     
		}
		}
               break;
           case EPOLL_ENGINE:
                 if ( event_count < 0){
                    fprintf(stderr,"Une erreur est survenue! dans le epoll du père\n");
                    break;
               }
               
               
            else  if ( event_count == -1) {
                   printf("epoll_wait");
                   exit(EXIT_FAILURE);
                   break;
              }
             else{
              	for(int i = 0; i < event_count; i++){
                 if (events[i].events & EPOLLIN){
                    if(!bytestosend){
		         ret = read(0, buffer, BUFFSIZE);
		         if(ret < 0)
		            fprintf(stderr, "Il ya eu un problème lors de la lecture\n");
		         else if(ret>0){
		            should_write = 1;
		            bytestosend += ret;
		            bytesread +=ret;
		            write(1, buffer, BUFFSIZE);
		         }
		         else {
		          fprintf(stderr, "end of father\n");
		          break;
		         }
		      }
		      else{
		         if(offset)
		             fprintf(stderr, "Peut on comblé le vide (%d)(%d)\n", bytestosend, offset);
		      }     
                 }
                 else if(events[i].events & EPOLLOUT){
                 
                    if (should_write){
		          ret = write(pfd[1], buffer+offset, bytestosend);
		          if(ret < 0){
		             fprintf(stderr, "il ya eu un problème lors de l'écriture %d\n", offset);
		             //break;
		          }
		          bytessent += ret;
		          bytestosend -= ret; 
		          offset +=  ret;
		          
		          if(bytestosend){
		             fprintf(stderr, "on a pas pu tout envoyer %d\n", bytestosend);
		          }else{
		            should_write = 0;
		            offset = 0;
		           fprintf(stderr, "on a tout envoyé %d\n", bytestosend);
		          //memset(buffer, 0, BUFFSIZE); 
		          }              
		       }
		      }
                 }
                
              }
              if(close(epoll_fd))
		  {
		    fprintf(stderr, "Impossible de fermer le descripteur de fichier epoll\n");
		    return 1;
		  }
		 
              break;
           default:
               break;
       
     
        }   
      }
   int status;
   fprintf(stderr, "waiting child bytesread (%ld), bytessent (%ld)\n", bytesread, bytessent);
   int pid2 = wait(&status);
  }
  return 0;
   
}

