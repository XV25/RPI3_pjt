#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <jpeglib.h>

static unsigned int width = 640;
static unsigned int height = 480;
static unsigned int fps = 30;
static int continuous = 0;
static unsigned char jpegQuality = 70;

//#include <SDL/SDL.h>
//#include <SDL/SDL_image.h>
//#include <SDL/SDL_ttf.h>
//#include <SDL/SDL_getenv.h>

static void jpegWrite(unsigned char* img, char* jpegFilename)
{
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        JSAMPROW row_pointer[1];
        FILE *outfile = fopen( jpegFilename, "wb" );

        // create jpeg data
        cinfo.err = jpeg_std_error( &jerr );
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, outfile);

        // set image parameters
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_YCbCr;

        // set jpeg compression parameters to default
        jpeg_set_defaults(&cinfo);
        // and then adjust quality setting
        jpeg_set_quality(&cinfo, jpegQuality, TRUE);

        // start compress
        jpeg_start_compress(&cinfo, TRUE);

        // feed data
        while (cinfo.next_scanline < cinfo.image_height) {
                row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        // finish compression
        jpeg_finish_compress(&cinfo);

        // destroy jpeg data
        jpeg_destroy_compress(&cinfo);

        // close output file
        fclose(outfile);
}



int main(int argc , char ** argv)
{
 
    pid_t pid;
    int id, portno, addServeur;
    char msg[255];//variable qui contiendrat les messages
    int width = 640, height = 480;

    char *img = malloc(width*height*3*sizeof(char));
 
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
        //recv(socketID, msg, 255, 0);
       // printf ("Image reçue : %s\n", msg);

        // recv(socketID, width, 255, 0);
        // recv(socketID, height, 255, 0);
        recv(socketID, img, strlen(img), 0);
        printf("Picture received!");
        jpegWrite(img,"client.jpg");
        free(img);
        img = malloc(width*height*3*sizeof(char));
        //printf("Je viens de recevoir l'image : %s \n", img);//  , img);


    }
    while (strcmp(msg, "aurevoir") != 0);    // tant que le client n'envoie pas "aurevoir" la conversation n'est pas fini
 
    shutdown(socketID, SHUT_RDWR);// fermeture du socket
 
    return 0;
 
}



// Fonction affichage image avec SDL 2
//void afficherImage(){

//SDL_Surface *ecran = NULL, *fond = NULL;
//SDL_Rect positionFond;
 
//    SDL_Event event;
 
//    int continuer = 1;
 
//    putenv("SDL_VIDEO_CENTERED=1");
//    SDL_Init(SDL_INIT_VIDEO);
 
//    ecran = SDL_SetVideoMode(800, 640, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
//    SDL_WM_SetCaption("SDLapp", NULL);
 
//fond = IMG_Load("index.jpeg");
 
//    positionFond.x = 0;
//    positionFond.y = 0;
 
//     while (continuer)
//    {
 
//        SDL_PollEvent(&event);
//        switch (event.type)
//        {
//        case SDL_QUIT:
//            continuer = 0;
//            break;
//        }
 
//        SDL_FillRect(ecran, NULL, SDL_MapRGB(ecran->format, 0, 0, 0));
 
//SDL_BlitSurface(fond, NULL, ecran, &positionFond);
 
//        SDL_Flip(ecran);
//    }
 
//SDL_FreeSurface(fond);
 
 
//    SDL_Quit();
 
//    return EXIT_SUCCESS;

//}
