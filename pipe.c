#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>
 

int main(int argc, char *argv[]){
  int fd[2], ret;
  pid_t pid;
  /* demande de création du pipe au système*/
  ret = pipe(fd);
  if(ret < 0){ /* Une erreur s'est produite lors de la création du pipe*/
    fprintf(stderr, "Erreur de création du pipe (%d)\n", errno);
    return 1;
  }
  pid = fork(); /* demande de création du processus fils*/
  if(pid < 0){ /* Une erreur s'est produite lors de la création du processus fils*/
    fprintf(stderr, "Erreur de création du fils(%d)\n", errno);
    return 1;
  }
  if(pid == 0){ /*Nous sommes dans le fils*/
    close(fd[1]); /* Le fils ferme l'extrémité d'écriture du pipe*/
     //fprintf(stderr,  "Nous sommes dans le fils(%d)\n", errno);
     char buffer[255];
     while(1){ /* Le fils se met en attente de lecture des données du pipe*/
      read(fd[0], buffer, 255);
      
      printf("Fils: Merci(%s)\n", buffer);
      if(strcmp(buffer, "N")==0) /* L'utilisateur met fin au programme*/
        break;
    }
    return 1;
  }
  else{
    close(fd[0]); /* Le père ferme l'extrémité de lecture du pipe*/
    // fprintf(stderr,  "Nous sommes dans le père(%d)\n", errno);
    fprintf(stdout, "Je suis le père\n");
     fprintf(stderr,  "Donnez moi votre nom (%d)\n", errno);
     char buffer[10];
     while(1){
     int n= scanf("%s", buffer); /* Le père lit une chaine de caractère saisie au clavier*/
     write(fd[1], buffer, n); /* le père écrit la chaine de caractère dans le pipe */
     if(strcmp(buffer, "N")==0) /* L'utilisateur met fin au programme*/
        break;
    }
    return 1;
    
  } 
  
  //Début du select
  fd_set readfds;       //On déclare un ensemble de descripteurs (pour les lectures)
  fd_set writefds;       //.. popur les écrituress
  FD_ZERO(&readfds); //initialisation du set
  FD_ZERO(&writefds);
  FD_SET(fd[0], &readfds); //ajout du descripteur du clavier dans l'ensemble des descripteurs de lecture
  FD_SET(fd[1], &writefds);
  
  char buff[10];
  int rets;        //Valeur de retour de select
  while(1){
  
  rets = select(2, &readfds, &writefds, NULL, NULL);
  
  if (rets < 0)
    {
        printf("Erreur de select\n");
    }
     
  if (FD_ISSET(fd[0], &readfds)){
            printf("Yo les gars des données sont dispo, venez vite les récupérer\n");
            rets = read(fd[0], buff, 10);
            if(rets>0)
            	fprintf(stdout, "%s\n",buff);
        }
   
    
  
    
  if(FD_ISSET(fd[1], &writefds))
        {
        	rets = write(1, "ça va aller", 255); // C'est à cause de cette ligne qu'il y a des caractères biza
            //rres à l'écran. Il tente d'extraire des données du buffer alors que ce dernier ne contient rien
        	if (rets  > 0)
        	{
        		fprintf(stderr, "Ecriture de données\n");
        	}
        	 
        }
        FD_CLR(0, &readfds);
        FD_CLR(1, &writefds);
     }
     int status;
     int pid2 = wait(&status); /* Le père attend la fin du fils. */
}
