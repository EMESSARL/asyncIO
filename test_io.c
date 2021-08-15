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

#define BUFFSIZE 131072
//#define false 0;

int main(int argc, char *argv[]){

   int pfd[2]; //pour le pipe
   pid_t pid;
   fd_set readset, writeset;
   char buff[BUFFSIZE];
   char buffer[BUFFSIZE];
   int ret, ret_sel,fd_max, timeout = 0, bytestosend = 0;
   int offset = 0, bytesrecved = 0;
   int should_write = 0;
   FD_ZERO(&readset);
   FD_ZERO(&writeset);
   struct timeval out_time;
   char c;
   bool with_time = false;

   out_time.tv_sec = 3;
   out_time.tv_usec = 0;
   
   fcntl(0, F_SETFL, O_NONBLOCK );
   fcntl(1, F_SETFL, O_NONBLOCK );
   
   memset(buffer, 0, BUFFSIZE);
   
   while ((c = getopt(argc, argv, "t")) != -1){
      switch (c)
      {
         case 't':
            with_time = true;
            break;
         default:
            break;
      }
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
      close(pfd[1]); //fermer l'extrémité de lecture pour éviter des comportements bizarres

      while (1){
         FD_SET(1, &writeset);
         fd_max = 1;
         FD_SET(pfd[0], &readset);
         if (pfd[0] > fd_max)
            fd_max = pfd[0];
         ret_sel = select(fd_max+1, &readset, &writeset, NULL, &out_time);

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
         
      }//Fin while du select dans le fils
      close(pfd[0]);
      exit(0);
   }//FIn du if (si on est dans le fils)

   else
  {
     long bytessent = 0;
     long bytesread = 0;
     while(1){
        fd_max = 0;
        FD_SET(0, &readset);
        FD_SET(pfd[1], &writeset);
        if (pfd[1] > fd_max)
           fd_max = pfd[1];
        
        if (with_time)
           ret_sel = select(fd_max+1, &readset, &writeset, NULL, &out_time);
        else
           ret_sel = select(fd_max+1, &readset, &writeset, NULL, NULL);
        
        if (ret_sel < 0){
            fprintf(stderr,"Une erreur est survenue! dans le select du père\n");
            break;
        }
        else if (ret_sel == 0){
            fprintf(stderr,"Temps expiré, rien à lire, rien à écrire dans le père!\n");
            break;
        }
        else{
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
                  fprintf(stderr, "end of father\n");
                  break;
                 }
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
                  //memset(buffer, 0, BUFFSIZE); 
                  }              
               }
            }
            
         }
         
         FD_CLR(0, &readset);
         FD_CLR(pfd[1], &writeset);
      }
   int status;
   fprintf(stderr, "waiting child bytesread (%ld), bytessent (%ld)\n", bytesread, bytessent);
   int pid2 = wait(&status);
  }
  return 0;
   
}
