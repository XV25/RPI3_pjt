#include "comm.h"
#include "util.h"
#include "util.c"

#define RESOLUTION 1

// Taille max en chaine de caractères d'une adresse ip
const int TAILLE_ADRESSE_IP = 13;

int definirResolution(int socketID, int x, int y){
    char * charx = malloc(sizeof(4));
    char * chary = malloc(sizeof(4));
    sprintf(charx, "%d", x);
    sprintf(chary, "%d", y);

    char * res = concat(charx, "/");
    res = concat(res, chary);

    return envoyerMessage(socketID, CMD_DEFINIR_RESOLUTION, res);
} 

int initComm(){
    struct sockaddr_in informations;  //structure donnant les informations sur le serveur
 
    /*initialisation du protocole, TCP  l'adresse de connection 127.0.0.1 (en local) et du port du serveur (1400)*/
    informations.sin_family = AF_INET;
    informations.sin_port = htons(2500);

    char * ip = malloc(sizeof(TAILLE_ADRESSE_IP));
    // Recuperation de l'adresse ip
    printf("\nAdresse IP de la caméra IP: "); 
    scanf("%s", ip);

    informations.sin_addr.s_addr = inet_addr(ip);
 
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

    return socketID;
}

int envoyerMessage(int socketID, char commande, char * msg){
    printf("Envoi du message : %s", msg);

    if ((send(socketID, msg, strlen(msg), 0)) == -1){
           perror("send");
           return 0;
    }
    return 1;
}

char * concat(char * dest, char * source){
	char *message = (char *)malloc(256);
    strcpy(message, dest);
    strcat(message, source);
    return message;
}