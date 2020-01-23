#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SUCCESS '0'
#define ERROR '1'
#define TAILLE_IMAGE '5'

#define CMD_PRENDRE_PHOTO '2'
#define CMD_DEFINIR_RESOLUTION '3'
#define INFO '4'

/**
 * Initialise la connexion avec le serveur
 * @return socketID
 */

int initComm();

/**
 * Définit la résolution de la capture
 * @param int x
 * @param int y
 * @return 0 si succes et 1 si echec
 */

int definirResolution(int socketID, int x, int y);

/**
* Envoyer un message au serveur
* @return 0 si succes et 1 si echec
*/
int envoyerMessage(int socketID, char commande, char * msg);

char * concat(char * dest, char * source);