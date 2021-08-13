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

//#define false 0;

int main(int argc, char *argv[]){

   int pfd[2]; //pour le pipe
   pid_t pid;
   fd_set readset, writeset;
   char buff[255];
   char buffer[BUFSIZ];
   int ret, ret_sel,fd_max;
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
   

   while ((c = getopt(argc, argv, "t")) != -1)
   {
      switch (c)
      {
         case 't':
            //fprintf(stderr,"Je rcois un param ici\n");
            with_time = true;
            //fprintf(stderr,"Je l'ai changé\n");
            break;
         default:
            break;
      }
   }



     // close(pfd[0]); //fermer la lecture du pipe

      /*Le père va écrire dans le pipe. On vérifie que le descripteur d'écriture est donc prêt*/
                  /*Création du pipe*/

   if ((pipe(pfd) == -1))
   {
      //printf("Erreur lors de la création du pipe\n");
      return 1;
   }
   
   fcntl(pfd[0], F_SETFL, O_NONBLOCK);
   fcntl(pfd[1], F_SETFL, O_NONBLOCK);

   /*Créer un processus fils pour lire dans notre pipe*/
   if ((pid = fork()) == -1)
   {
      //printf("Echec de la création du processus fils\n");
      return 1;
   }

   if (pid == 0)
   {
      // FD_SET(1, &writeset);
      // FD_SET(pfd[1], &writeset);
      fd_max = 0;
      close(pfd[1]); //fermer l'extrémité de lecture pour éviter des comportements bizarres

      while (1)
      {
         //FD_SET(0, &readset);
         FD_SET(pfd[0], &readset);
         if (pfd[0] > fd_max)
         {
            fd_max = pfd[0];
         }
         

         ret_sel = select(fd_max+1, &readset, NULL, NULL, &out_time);

         if (ret_sel < 0)
         {
            //fprintf(stderr, "Erreur du select dans le fils\n");
         }
         else if(ret_sel == 0)
         {
            //fprintf(stderr, "Je suius fatigué d'attendre dans le fils\n");
            sleep(10);
         }
         else
         {
            if (FD_ISSET(pfd[0], &readset))
            {
               int n = read(pfd[0],buffer, BUFSIZ) !=0;
               printf("%s", buffer);
               if (strcmp(buffer, "N") == 0)
               {
                  close(pfd[0]);
                  break;
               }
            } 
               
         }
            FD_CLR(pfd[0], &readset);
            //FD_CLR(pfd[1], &writeset);      
         
      }//Fin while du select dans le fils
      
   }//FIn du if (si on est dans le fils)

   else
  {
     while(1){
        fd_max = 0;
        FD_SET(0, &readset);
        FD_SET(pfd[1], &writeset);
        if (pfd[1] > fd_max)
        {
           fd_max = pfd[1];
        }
        if (with_time)
        {
            //fprintf(stderr, "je suis dans le if\n");
           ret_sel = select(fd_max+1, &readset, &writeset, NULL, &out_time);
        }
         else
         {
            //fprintf(stderr,"Je suis dansle else plutot\n");
            sleep(10);
            ret_sel = select(fd_max+1, &readset, &writeset, NULL, NULL);
         }

         if (ret_sel < 0)
         {
            //fprintf(stderr,"Une erreur est survenue! dans le select du père\n");
         }
         else if (ret_sel == 0)
         {
            //fprintf(stderr,"Temps expiré, rien à lire, rien à écrire!\n");
            break;
            sleep(10);
         }
         else
         {
            if(FD_ISSET(0, &readset)){
               //fprintf(stderr, "Quelque chose à lire\n");
               ret = read(0, buffer, 255);
               if(ret>0)
               {
                  should_write = 1;
                  //fprintf(stderr, "%s", buffer);
               }
               else if(ret == 0)
               {
                  write(pfd[1], "N", 1);
                  break;
               }
            }

            if (FD_ISSET(pfd[1], &writeset))
            {
               
               //scanf("%s", buffer);
               if (should_write)
               {
                  //printf("Yo les gars entrez des données pour notre pipe!\n");
                  write(pfd[1], buffer, strlen(buffer));
        
                  should_write = 0;
               }
            }
            
         }
         
         FD_CLR(0, &readset);
         FD_CLR(pfd[1], &writeset);
      }
  }
   //int status;
   //int pid2 = wait(&status);
}
