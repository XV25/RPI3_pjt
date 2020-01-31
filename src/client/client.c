#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <jpeglib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_getenv.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

static unsigned int width = 640;
static unsigned int height = 480;
static unsigned int fps = 30;
static int continuous = 0;
static unsigned char jpegQuality = 70;
static int nb_pic = 0;




static void jpegWrite(unsigned char* img, char* jpegFilename)
{
	
	char * oldjpegFilename = malloc(strlen(jpegFilename)*sizeof(char) );
	strcpy(oldjpegFilename,jpegFilename);

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        JSAMPROW row_pointer[1];
	char * str = malloc(3*sizeof(char) );
	sprintf(str,"%d",nb_pic);

        strcat(oldjpegFilename,str);
        strcat(oldjpegFilename,".jpg");

	int removeFile = remove(oldjpegFilename);
	if (removeFile != 0)
	{
		fprintf( stderr, "Error: cannot remove the file.\n" );
	}

	nb_pic++;
	char * newjpegFilename = malloc(strlen(jpegFilename)*sizeof(char) );
	strcpy(newjpegFilename,jpegFilename);
	char * str2 = malloc(3*sizeof(char) );

	sprintf(str2,"%d",nb_pic);

	strcat(newjpegFilename,str2);
	strcat(newjpegFilename,".jpg");

        FILE *outfile = fopen( newjpegFilename, "wb" );

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

        printf("Picture_written at : %s",newjpegFilename);
        // finish compression
        jpeg_finish_compress(&cinfo);

        // destroy jpeg data
        jpeg_destroy_compress(&cinfo);

        // close output file
        fclose(outfile);

}

static void AvailableCommands()
{
        printf("---------- AvailableCommands -----------\n");
        printf("Take picture : %d \n",1);
        printf("Quit : %s \n","bye");
        printf("Help : %s \n","h");

        }


int main(int argc , char ** argv)
{
 
    pid_t pid;
    int id, portno, addServeur;
   // char msg[255];//variable qui contiendrat les messages
char * msg = malloc(255*sizeof(char) );
    int width = 640, height = 480;

    unsigned char *img = malloc(width*height*3*sizeof(char));
 
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
 
    if (strcmp(msg, "bye") != 0)
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
 
        if (strcmp(msg,"1") == 0)
        {

        if ((send(socketID, msg, strlen(msg), 0)) == -1)
            perror("send");

        recv(socketID, img, width*height*3*sizeof(char), 0);

	free(msg);
	msg = malloc(255*sizeof(char) );
	
	strcpy(msg,"Picture_received\n");
	printf(msg);

	send(socketID,msg,255*sizeof(char),0);
        jpegWrite(img,"client");
        afficherImage(img);
        free(img);
        img = malloc(width*height*3*sizeof(char));
        }

        else if (strcmp(msg,"h") == 0)
        {
            AvailableCommands();
        }

        else if (strcmp(msg,"bye") == 0)
        {
            printf("Quit program\n");
            if ((send(socketID, msg, strlen(msg), 0)) == -1)
                perror("send");
            break;
        }

        else {
            printf("Error : command not recognized.\n");
        }
        free(msg);
        msg = malloc(255*sizeof(char) );
    }
    while (1);


    shutdown(socketID, SHUT_RDWR);// fermeture du socket
 
    return 0;
 
}



//// Fonction affichage image avec SDL 2
void afficherImage(char * data){
    int req_format = STBI_rgb_alpha;

    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  int shift = (req_format == STBI_rgb) ? 8 : 0;
  rmask = 0xff000000 >> shift;
  gmask = 0x00ff0000 >> shift;
  bmask = 0x0000ff00 >> shift;
  amask = 0x000000ff >> shift;
#else // little endian, like x86
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
#endif

int depth, pitch;
if (req_format == STBI_rgb) {
  depth = 24;
  pitch = 3*width; // 3 bytes per pixel * pixels per row
} else { // STBI_rgb_alpha (RGBA)
  depth = 32;
  pitch = 4*width;
}


SDL_Surface *ecran = NULL;
SDL_Surface *fond = SDL_CreateRGBSurfaceFrom((void*)data, width, height, depth, pitch,
                                             rmask, gmask, bmask, amask);

SDL_Rect positionFond;
 
    SDL_Event event;
 
    int continuer = 1;
 
    putenv("SDL_VIDEO_CENTERED=1");
    SDL_Init(SDL_INIT_VIDEO);
 
    ecran = SDL_SetVideoMode(800, 640, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption("SDLapp", NULL);
 
//fond = IMG_Load("index.jpeg");
 
    positionFond.x = 0;
    positionFond.y = 0;
 
     while (continuer)
    {
 
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            continuer = 0;
            break;
        }
 
        SDL_FillRect(ecran, NULL, SDL_MapRGB(ecran->format, 0, 0, 0));
 
SDL_BlitSurface(fond, NULL, ecran, &positionFond);
 
        SDL_Flip(ecran);
    }
 
SDL_FreeSurface(fond);
 
 
    SDL_Quit();
 
    return EXIT_SUCCESS;

}
