#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void){

    fd_set readfds;       //On déclare un ensemble de descripteurs (pour les lectures)
    fd_set writefds;       //.. popur les écrituress
    struct timeval outime; //pour définir l'interval de temps pendant lequel select doit attendre
    int ret;        //Valeur de retour de select

    FD_ZERO(&readfds); //initialisation du set
    FD_ZERO(&writefds);
    FD_SET(0, &readfds); //ajout du descripteur du clavier dans l'ensemble des descripteurs de lecture
    FD_SET(1, &writefds);

    /*On va définir notre délai. Ici on choisit d'attendre pendant 10s*/
    outime.tv_sec = 10; //Valeur en secondes
    outime.tv_usec = 0; //valeur en microsecondes

    char buff[255];

    ret = select(2, &readfds, &writefds, NULL, &outime);

    if (ret < 0)
    {
        printf("Erreur de select\n");
    }else if(ret == 0){
        printf("Le temps s'est écoulé mon pote\n");
    }else{
        printf("Yo les gars des données sont dispo, venez vite les récupérer\n");
        if (FD_ISSET(0, &readfds))
        {
            ret = read(0, buff, 255);
            if (ret > 0)
            {
                fprintf(stderr, "%s", buff);
            }
            EXIT_SUCCESS;
            
        }
        /*by celia*/
         if(FD_ISSET(1, &writefds))
        {
        	ret = write(1, buff, 255);
        	if (ret  > 0)
        	{
        		fprintf(stderr, "Ecriture de données\n");
        	}
        	EXIT_SUCCESS;
        }
        
    }
    



}
