#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdbool.h>

//#define false 0;

int main(int argc, char *argv[]){
   
   fd_set readset, writeset;
   char buff[255];
   int ret, ret_sel;
   int should_write = 0;
   FD_ZERO(&readset);
   FD_ZERO(&writeset);
   FD_SET(0, &readset);
   FD_SET(1, &writeset);
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
      FD_SET(1, &writeset);
      if (with_time)
      {
      //   fprintf(stderr, "je suis dans le if\n");
         ret_sel = select(2, NULL, &writeset, NULL, &out_time);
      }else{
         fprintf(stderr,"Je suis dansle else plutot\n");
         sleep(10);
         ret_sel = select(2, &readset, &writeset, NULL, NULL);
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
      }
   }
}
