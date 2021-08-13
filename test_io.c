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
   int ret, ret_sel;
   int should_write = 0;
   FD_ZERO(&readset);
   FD_ZERO(&writeset);
   // FD_SET(0, &readset);
   // FD_SET(pfd[0], &readset);
   // FD_SET(pfd[1], &writeset);
   //FD_SET(1, &writeset);
   struct timeval out_time;
   char c;
   bool with_time = false;

   out_time.tv_sec = 3;
   out_time.tv_usec = 0;
   
   fcntl(0, F_SETFL, O_NONBLOCK );
   fcntl(1, F_SETFL, O_NONBLOCK );
   fcntl(pfd[0], F_SETFL, O_NONBLOCK);
   fcntl(pfd[1], F_SETFL, O_NONBLOCK);


      close(pfd[0]); //fermer la lecture du pipe

      /*Le père va écrire dans le pipe. On vérifie que le descripteur d'écriture est donc prêt*/
                  /*Création du pipe*/

   if ((pipe(pfd) == -1))
   {
      printf("Erreur lors de la création du pipe\n");
      return 1;
   }

   /*Créer un processus fils pour lire dans notre pipe*/
   if ((pid = fork()) == -1)
   {
      printf("Echec de la création du processus fils\n");
      return 1;
   }

   if (pid == 0)
   {
      FD_SET(0, &readset);
      FD_SET(pfd[0], &readset);
     // FD_SET(1, &writeset);
      FD_SET(pfd[1], &writeset);

      ret_sel = select(3, &readset, &writeset, NULL, &out_time);
      close(pfd[1]); //fermer l'extrémité de lecture pour éviter des comportements bizarres

       /*Le processus fils va lire dans le pipe Mais avant faudra vérifier que le descripteur lecture du pipe est prêt*/

      if (FD_ISSET(pfd[0], &readset))
      {
         while (1)
         {
            int n = read(pfd[0],buffer, BUFSIZ) !=0;
            printf("Le fils a lu: %s\n", buffer);
            if (strcmp(buffer, "N") == 0)
            {
               break;
            }
         } 
         close(pfd[0]);
      }
      FD_CLR(pfd[0], &readset);
      FD_CLR(pfd[1], &writeset);
      
   }else
  {
      
      while ((c = getopt(argc, argv, "t")) != -1)
      {
         switch (c)
         {
         case 't':
         fprintf(stderr,"Je rcois un param ici\n");
            with_time = true;
            fprintf(stderr,"Je l'ai changé\n");
            break;
         
         default:
            break;
         }
      }
      

      while(1){
         FD_SET(0, &readset);
         FD_SET(pfd[1], &writeset);
      // FD_SET(1, &writeset);
         if (with_time)
         {
         //   fprintf(stderr, "je suis dans le if\n");
            ret_sel = select(3, &readset, &writeset, NULL, &out_time);
         }else{
            fprintf(stderr,"Je suis dansle else plutot\n");
            sleep(10);
            ret_sel = select(3, &readset, &writeset, NULL, NULL);
         }


         if (FD_ISSET(pfd[1], &writeset))
         {
            while (1)
            {
               printf("Veuillez saisir des données pour notre pipe\n");
               scanf("%s", buffer);
               write(pfd[1], buffer, strlen(buffer));
               if (strcmp(buffer, "N") ==0)
               {
                  break;
               }
            }
            close(pfd[1]); //fermeture du pipe
         }


         if (ret_sel < 0)
         {
            fprintf(stderr,"Une erreur est survenue!\n");
         }else if (ret_sel == 0)
         {
            fprintf(stderr,"Temps expiré, rien à lire, rien à écrire!\n");
            break;
            sleep(500);
         }else
         {
            if(FD_ISSET(0, &readset)){
               fprintf(stderr, "Quelque chose à lire\n");
               ret = read(0, buff, 255);
               if(ret>0)
                  fprintf(stderr, "%s", buff);
            }
            if(FD_ISSET(1, &writeset)){
               if(should_write){ //Cette condition est elle vraiment nécessaire???
                  fprintf(stderr, "Quelques chose à écrire\n");
                  int n = write(1, "Bonjour", 8);
                  if (n < 0){
                     fprintf(stderr, "Une erreur\n");
                     //while(1);
                  }
            }
         }
         
         FD_CLR(0, &readset);
         FD_CLR(1, &writeset);
         FD_CLR(pfd[0], &readset);
         FD_CLR(pfd[1], &writeset);
         }
      }
  }
}
