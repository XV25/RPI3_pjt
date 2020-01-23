#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
int main(int argc , char ** argv)
{
 
    pid_t pid;
    int id, portno, addServeur;
    char msg[255];//variable qui contiendrat les messages

int width;
int height;

char * cwidth;
char * cheight;
 
    struct sockaddr_in informations;  //structure donnant les informations sur le serveur


    // Test du nombre d'arguments 
    if (argc < 3)
    {
        fprintf(stderr,"Spécifiez les arguments SVP %s nom_hôte No_Port\n", argv[0]);
        exit(0);
     }
    portno = atoi(argv[2]); // converti une chaine de caractére a un entier
    addServeur = atoi(argv[1]);

    /*initialisation du protocole, TCP  l'adresse de connection 127.0.0.1 (en local) et du port du serveur (1400)*/
    informations.sin_family = AF_INET;
    informations.sin_port = htons(portno);
    informations.sin_addr.s_addr = INADDR_ANY;
 
    int socketID = socket(AF_INET, SOCK_STREAM, 0); // creation du socket propre au client
 
    if (socketID == -1)    //test de création du socket
    {
        perror("socket");
        exit (-1);
    }
 
    if ((connect(socketID, (struct sockaddr *) &informations, sizeof(struct sockaddr_in))) == -1)   //connexion au serveur
    {
        perror("connect");
        exit (-1);
    }
 
    if (strcmp(msg, "aurevoir") != 0)
    {
        memset(msg, 0, 255);
        recv(socketID, msg, 255, 0);
        printf ("%s\n", msg);
    }
 
    do
    {
        id+=1;
        printf ("\n moi : ");
        fgets(msg, 255, stdin);// le client ecrit son message
        msg[strlen(msg) - 1] = '\0';
 
        if ((send(socketID, msg, strlen(msg), 0)) == -1)
            perror("send");
        recv(socketID, cwidth, 255, 0);
        recv(socketID, cheight, 255, 0);

    char* img = malloc(width*height*3*sizeof(char));
        recv(socketID, img, width*height*3, 0);

       // recv(socketID, msg, 255, 0);
        printf ("Image reçue : %s\n", img);

        // recv(socketID, width, 255, 0);
        // recv(socketID, height, 255, 0);
        // recv(socketID, img, 255, 0);

    }
    while (strcmp(msg, "aurevoir") != 0);    // tant que le client n'envoie pas "aurevoir" la conversation n'est pas fini
 
    shutdown(socketID, SHUT_RDWR);// fermeture du socket
 
    return 0;
 
}


// void afficherImage(){

//     SDL_Surface *ecran = NULL, *imageDeFond = NULL;
//     SDL_Rect positionFond;

//     positionFond.x = 0;
//     positionFond.y = 0;

//     SDL_Init(SDL_INIT_VIDEO);

//     ecran = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
//     SDL_WM_SetCaption("Chargement d'images en SDL", NULL);

//     /* Chargement d'une image Bitmap dans une surface */
//     imageDeFond = SDL_LoadBMP("lac_en_montagne.bmp");
//     /* On blitte par-dessus l'écran */
//     SDL_BlitSurface(imageDeFond, NULL, ecran, &positionFond);

//     SDL_Flip(ecran);
//     pause();

//     SDL_FreeSurface(imageDeFond); /* On libère la surface */
//     SDL_Quit();

//     return EXIT_SUCCESS;

// }
