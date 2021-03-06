﻿/***************************************************************************
 *   v4l2grab Version 0.3                                                  *
 *   Copyright (C) 2012 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   based on V4L2 Specification, Appendix B: Video Capture Example        *
 *   (http://v4l2spec.bytesex.org/spec/capture-example.html)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
 /**************************************************************************
 *   Modification History                                                  *
 *                                                                         *
 *   Matthew Witherwax      21AUG2013                                      *
 *      Added ability to change frame interval (ie. frame rate/fps)        *
 * Martin Savc              7JUL2015
 *      Added support for continuous capture using SIGINT to stop.
 ***************************************************************************/

// compile with all three access methods
#if !defined(IO_READ) && !defined(IO_MMAP) && !defined(IO_USERPTR)
#define IO_READ
#define IO_MMAP
#define IO_USERPTR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "./lib/comm.h"
#include "./lib/lblink.h"
 
#include "config.h"
#include "yuv.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#ifndef VERSION
#define VERSION "unknown"
#endif

#if defined(IO_MMAP) || defined(IO_USERPTR)
// minimum number of buffers to request in VIDIOC_REQBUFS call
#define VIDIOC_REQBUFS_COUNT 2
#endif

typedef enum {
#ifdef IO_READ
        IO_METHOD_READ,
#endif
#ifdef IO_MMAP
        IO_METHOD_MMAP,
#endif
#ifdef IO_USERPTR
        IO_METHOD_USERPTR,
#endif
} io_method;

struct buffer {
        void *                  start;
        size_t                  length;
};

static io_method        io              = IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

// global settings
static unsigned int width = 640;
static unsigned int height = 480;
static unsigned int port = 2400;
static unsigned int fps = -1;
static int continuous = 0;
static unsigned char jpegQuality = 70;
static char* jpegFilename = NULL;
static char* jpegFilenamePart = NULL;
static char* deviceName = "/dev/video0";
static int running = 1;

static const char* const continuousFilenameFmt = "%s_%010"PRIu32"_%" PRId64".jpg";

/**
SIGINT interput handler
*/
// stop continuous
//void StopContCapture(int sig_id) {
//	printf("stoping continuous capture\n");
//	continuous = 0;
//}

void StopCapture(int sig_id) {
        printf("quitting server\n");
        running = 0;
        if (-1 == GPIOUnexport(POUT1) || -1 == GPIOUnexport(POUT2))
        {
                printf("Error");
                }
}

void InstallSIGINTHandler() {
	struct sigaction sa;
	CLEAR(sa);
	
        //sa.sa_handler = StopContCapture;
        sa.sa_handler = StopCapture;
	if(sigaction(SIGINT, &sa, 0) != 0)
	{
                fprintf(stderr,"could not install SIGINT handler, no running\n");
                running = 0;
	}
}

/**
	Print error message and terminate programm with EXIT_FAILURE return code.

	\param s error message to print
*/
static void errno_exit(const char* s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

/**
	Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.

	\param fd file descriptor
	\param request request
	\param argp argument
	\returns result from ioctl
*/
static int xioctl(int fd, int request, void* argp)
{
	int r;

	do r = v4l2_ioctl(fd, request, argp);
	while (-1 == r && EINTR == errno);

	return r;
}

/**
	Write image to jpeg file.

	\param img image to write
*/
static void jpegWrite(unsigned char* img, char* jpegFilename)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
	FILE *outfile = fopen( jpegFilename, "wb" );

	// try to open file for saving
	if (!outfile) {
		errno_exit("jpeg");
	}

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

/**
	process image read
*/
static void imageProcess(const void* p, struct timeval timestamp, unsigned char * img)
{
	//timestamp.tv_sec
	//timestamp.tv_usec
	unsigned char* src = (unsigned char*)p;
	unsigned char* dst = malloc(width*height*3*sizeof(char));

	YUV420toYUV444(width, height, src, dst);
// just save pic with different name
//	if(continuous==1) {
//		static uint32_t img_ind = 0;
//		int64_t timestamp_long;
//		timestamp_long = timestamp.tv_sec*1e6 + timestamp.tv_usec;
//		sprintf(jpegFilename,continuousFilenameFmt,jpegFilenamePart,img_ind++,timestamp_long);

//	}
	// write jpeg
    //jpegWrite(dst,jpegFilename);


     memcpy(img,dst,width*height*3*sizeof(char));


	// free temporary image
        //free(dst);
}

/**
	read single frame
*/
static int frameRead(unsigned char* img )
{

	struct v4l2_buffer buf;
#ifdef IO_USERPTR
	unsigned int i;
#endif

	switch (io) {
#ifdef IO_READ
		case IO_METHOD_READ:
			if (-1 == v4l2_read(fd, buffers[0].start, buffers[0].length)) {
				switch (errno) {
					case EAGAIN:
						return 0;

					case EIO:
						// Could ignore EIO, see spec.
						// fall through

					default:
						errno_exit("read");
				}
			}

			struct timespec ts;
			struct timeval timestamp;
			clock_gettime(CLOCK_MONOTONIC,&ts);
			timestamp.tv_sec = ts.tv_sec;
			timestamp.tv_usec = ts.tv_nsec/1000;

            imageProcess(buffers[0].start,timestamp, img);
			break;
#endif

#ifdef IO_MMAP
		case IO_METHOD_MMAP:
			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
					case EAGAIN:
						return 0;

					case EIO:
						// Could ignore EIO, see spec
						// fall through

					default:
						errno_exit("VIDIOC_DQBUF");
				}
			}

			assert(buf.index < n_buffers);



            imageProcess(buffers[buf.index].start,buf.timestamp,img);

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");

			break;
#endif

#ifdef IO_USERPTR
			case IO_METHOD_USERPTR:
				CLEAR (buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;

				if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
					switch (errno) {
						case EAGAIN:
							return 0;

						case EIO:
							// Could ignore EIO, see spec.
							// fall through

						default:
							errno_exit("VIDIOC_DQBUF");
					}
				}

				for (i = 0; i < n_buffers; ++i)
					if (buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
						break;

				assert (i < n_buffers);

                imageProcess((void *)buf.m.userptr,buf.timestamp, img);

				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
					errno_exit("VIDIOC_QBUF");
				break;
#endif
	}



	return 1;
}

/**
	mainloop: read frames and process them
*/
static void mainLoop(int port)
{	
	int count;
	unsigned int numberOfTimeouts;

	numberOfTimeouts = 0;
	count = 3;



    unsigned char* img = malloc(width*height*3*sizeof(char));

    struct sockaddr_in information_server;
    struct sockaddr_in information_client;

    int socketID = socket(AF_INET, SOCK_STREAM, 0);
    int connexion = 0;
    int pid;
    int id;

    char * msg = malloc(255*sizeof(char) );
    char * msg2 = malloc(255*sizeof(char) );
    id=0;

    fd_set fds;
    struct timeval tv;
    int r;

    socklen_t len = sizeof(struct sockaddr_in); //déclaration d' une variable du type socklen_t qui contiendra la taille de la structure

    if (socketID == -1)
    {
        perror("socket");
        exit(-1);
    }

    /*initialisation du protocole, TCP  l'adresse de connection 127.0.0.1 (en local) et du port du serveur (1400)*/
    memset(&information_server, 0, sizeof(struct sockaddr_in));
    information_server.sin_port = htons(port);
    information_server.sin_family = AF_INET;

    /* création de la connexion*/
    if ((bind(socketID, (struct sockaddr *) &information_server, sizeof(struct sockaddr))) == -1)
    {
        perror("bind");
        exit(-1);
    }
    printf("Working!");

    /* le serveur écoute si un client cherche à se connecter*/
    if ((listen(socketID, 5)) == -1)
    {
        perror("listen");
        exit (-1);
    }
    while (running)
    {

        memset(&information_client, 0, sizeof(struct sockaddr_in));
        connexion = accept(socketID, (struct sockaddr *) &information_client, &len); //le serveur accepte la connexion

        if (connexion == -1)
        {
            perror("accept");
            exit(-1);
        }
        id+=1;
        /* Create child process */
        pid = fork();

        if (pid < 0)
        {
            perror("ERROR on fork");
            exit(1);
        }
        if (pid == 0)
        {

            /* This is the client process */
            close(socketID);
            InstallSIGINTHandler();

            if (-1 == GPIOExport(POUT1))
                            printf("Error Exporting LED1 \n");
            if (-1 == GPIODirection(POUT1, OUT))
                printf("Error setting direction LED1 \n");
            if (-1 == GPIOWrite(POUT1, 1))
                    printf("Error setting value to LED1 \n");

            if (-1 == GPIOExport(POUT2))
                            printf("Error Exporting LED2 \n");
            if (-1 == GPIODirection(POUT2, OUT))
                printf("Error setting direction LED2 \n");

            printf ("Connexion acceptée de : client %i\n",id);
            memset(msg, 0, 255);
            sprintf(msg,"bienvenu! client %i",id);
            send(connexion, msg, strlen(msg), 0);
            do
            {
                memset(msg, 0, 255);
                recv(connexion, msg, 255, 0);

                if (strcmp(msg, "bye") == 0)    //si le client ecrit aurevoir il est deconnecté du chat
                {
                    printf ("Connexion fermée pour le client %i\n",id);
                    shutdown(socketID, SHUT_RDWR);
                    exit (0);
                }

            if (strcmp(msg, "1") == 0 )
            {
                int tst = 0;
                if (-1 == GPIOWrite(POUT2, 1))
                        printf("Error setting value to LED2 \n");
                while (tst == 0)
                {
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                /* Timeout. */
                tv.tv_sec = 1;
                tv.tv_usec = 0;

                r = select(fd + 1, &fds, NULL, NULL, &tv);

                if (-1 == r) {
                        if (EINTR == errno)
                                continue;

                        errno_exit("select");
                }
                if (frameRead(img))
                {
                    tst = 1;

                }

                }
                if (jpegFilename)
                {
                jpegWrite(img,jpegFilename);
                }

               // printf ("Picture took ! %s \n",img);
                send(connexion, img, width*height*3*sizeof(char), 0);
                printf ("client %d : %s\n",id,msg);

                //printf ("Picture send!");
		free(msg2);
		msg2 = malloc( 255*sizeof(char) );
		recv(connexion, msg2, 255*sizeof(char), 0);
		printf(msg2);
                if (-1 == GPIOWrite(POUT2, 0))
                        printf("Error setting value to LED2 \n");

            }


            free(img);
            img = malloc(width*height*3*sizeof(char));
		free(msg);
		msg = malloc(255*sizeof(char) );
            }
            while(running);
        }
        else
        {
            InstallSIGINTHandler();
//            if (-1 == GPIOUnexport(POUT1) || -1 == GPIOUnexport(POUT2))
//            {
//                    printf("Error");
//                    }

            close(connexion);
            connexion = -1;

        }

    }


//    if (-1 == GPIOUnexport(POUT1) || -1 == GPIOUnexport(POUT2))
//    {
//            printf("Error");
//            }


}

/**
	stop capturing
*/
static void captureStop(void)
{
        enum v4l2_buf_type type;

        switch (io) {
#ifdef IO_READ
                case IO_METHOD_READ:
                        /* Nothing to do. */
                        break;
#endif

#ifdef IO_MMAP
                case IO_METHOD_MMAP:
#endif
#ifdef IO_USERPTR
                case IO_METHOD_USERPTR:
#endif
#if defined(IO_MMAP) || defined(IO_USERPTR)
                        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");

                        break;
#endif
        }
}

/**
  start capturing
*/
static void captureStart(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
#ifdef IO_READ
                case IO_METHOD_READ:
                        /* Nothing to do. */
                        break;
#endif

#ifdef IO_MMAP
                case IO_METHOD_MMAP:
                        for (i = 0; i < n_buffers; ++i) {
                                struct v4l2_buffer buf;

                                CLEAR(buf);

                                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                buf.memory = V4L2_MEMORY_MMAP;
                                buf.index = i;

                                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                        errno_exit("VIDIOC_QBUF");
                                }

                        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                                errno_exit("VIDIOC_STREAMON");

                        break;
#endif

#ifdef IO_USERPTR
                case IO_METHOD_USERPTR:
                        for (i = 0; i < n_buffers; ++i) {
                                struct v4l2_buffer buf;

                        CLEAR (buf);

                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long) buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                        }

                        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                                errno_exit("VIDIOC_STREAMON");

                        break;
#endif
        }
        if (-1 == GPIOExport(POUT1))
                        printf("Error Exporting LED1 \n");
        if (-1 == GPIODirection(POUT1, OUT))
            printf("Error setting direction LED1 \n");
        if (-1 == GPIOWrite(POUT1, 1))
                printf("Error setting value to LED1 \n");


}

static void deviceUninit(void)
{
        unsigned int i;

        switch (io) {
#ifdef IO_READ
                case IO_METHOD_READ:
                        free(buffers[0].start);
                        break;
#endif

#ifdef IO_MMAP
                case IO_METHOD_MMAP:
                        for (i = 0; i < n_buffers; ++i)
                                if (-1 == v4l2_munmap(buffers[i].start, buffers[i].length))
                                        errno_exit("munmap");
                        break;
#endif

#ifdef IO_USERPTR
                case IO_METHOD_USERPTR:
                        for (i = 0; i < n_buffers; ++i)
                                free(buffers[i].start);
                        break;
#endif
        }

        free(buffers);
}

#ifdef IO_READ
static void readInit(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf (stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}
#endif

#ifdef IO_MMAP
static void mmapInit(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = VIDIOC_REQBUFS_COUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support memory mapping\n", deviceName);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = v4l2_mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}
#endif

#ifdef IO_USERPTR
static void userptrInit(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;
        unsigned int page_size;

        page_size = getpagesize();
        buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

        CLEAR(req);

        req.count = VIDIOC_REQBUFS_COUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support user pointer i/o\n", deviceName);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = memalign(/* boundary */ page_size, buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}
#endif

/**
        initialize device
*/
static void deviceInit(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        struct v4l2_streamparm frameint;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",deviceName);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",deviceName);
                exit(EXIT_FAILURE);
        }

        switch (io) {
#ifdef IO_READ
                case IO_METHOD_READ:
                        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                                fprintf(stderr, "%s does not support read i/o\n",deviceName);
                                exit(EXIT_FAILURE);
                        }
                        break;
#endif

#ifdef IO_MMAP
                case IO_METHOD_MMAP:
#endif
#ifdef IO_USERPTR
                case IO_METHOD_USERPTR:
#endif
#if defined(IO_MMAP) || defined(IO_USERPTR)
                        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                                fprintf(stderr, "%s does not support streaming i/o\n",deviceName);
                                exit(EXIT_FAILURE);
                        }
                        break;
#endif
        }

        /* Select video input, video standard and tune here. */
        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                                case EINVAL:
                                        /* Cropping not supported. */
                                        break;
                                default:
                                        /* Errors ignored. */
                                        break;
                        }
                }
        } else {
                /* Errors ignored. */
        }

        CLEAR(fmt);

        // v4l2_format
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420) {
                fprintf(stderr,"Libv4l didn't accept YUV420 format. Can't proceed.\n");
                exit(EXIT_FAILURE);
        }

        /* Note VIDIOC_S_FMT may change width and height. */
        if (width != fmt.fmt.pix.width) {
                width = fmt.fmt.pix.width;
                fprintf(stderr,"Image width set to %i by device %s.\n", width, deviceName);
        }

        if (height != fmt.fmt.pix.height) {
                height = fmt.fmt.pix.height;
                fprintf(stderr,"Image height set to %i by device %s.\n", height, deviceName);
        }

  /* If the user has set the fps to -1, don't try to set the frame interval */
  if (fps != -1)
  {
    CLEAR(frameint);

    /* Attempt to set the frame interval. */
    frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frameint.parm.capture.timeperframe.numerator = 1;
    frameint.parm.capture.timeperframe.denominator = fps;
    if (-1 == xioctl(fd, VIDIOC_S_PARM, &frameint))
      fprintf(stderr,"Unable to set frame interval.\n");
  }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
#ifdef IO_READ
                case IO_METHOD_READ:
                        readInit(fmt.fmt.pix.sizeimage);
                        break;
#endif

#ifdef IO_MMAP
                case IO_METHOD_MMAP:
                        mmapInit();
                        break;
#endif

#ifdef IO_USERPTR
                case IO_METHOD_USERPTR:
                        userptrInit(fmt.fmt.pix.sizeimage);
                        break;
#endif
        }
}

/**
        close device
*/
static void deviceClose(void)
{
        if (-1 == v4l2_close(fd))
                errno_exit("close");

        fd = -1;
}

/**
	open device
*/
static void deviceOpen(void)
{
	struct stat st;

	// stat file
	if (-1 == stat(deviceName, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// check if its device
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", deviceName);
		exit(EXIT_FAILURE);
	}

	// open device
	fd = v4l2_open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

	// check if opening was successfull
	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/**
	print usage information
*/
static void usage(FILE* fp, int argc, char** argv)
{
	fprintf(fp,
		"Usage: %s [options]\n\n"
		"Options:\n"
		"-d | --device name   Video device name [/dev/video0]\n"
		"-h | --help          Print this message\n"
                "-o | --output        Set JPEG output filename in server side\n"
		"-q | --quality       Set JPEG quality (0-100)\n"
		"-m | --mmap          Use memory mapped buffers\n"
		"-r | --read          Use read() calls\n"
		"-u | --userptr       Use application allocated buffers\n"
                //"-W | --width         Set image width\n"
                //"-H | --height        Set image height\n"
		"-I | --interval      Set frame interval (fps) (-1 to skip)\n"
                //"-c | --continuous    Do continous capture, stop with SIGINT.\n"
		"-v | --version       Print version\n"
                "-P | --port          Number port\n"
		"",
		argv[0]);
	}

static const char short_options [] = "d:ho:q:mruW:H:P:I:v";

static const struct option
long_options [] = {
	{ "device",     required_argument,      NULL,           'd' },
	{ "help",       no_argument,            NULL,           'h' },
	{ "output",     required_argument,      NULL,           'o' },
	{ "quality",    required_argument,      NULL,           'q' },
	{ "mmap",       no_argument,            NULL,           'm' },
	{ "read",       no_argument,            NULL,           'r' },
	{ "userptr",    no_argument,            NULL,           'u' },
        //{ "width",      required_argument,      NULL,           'W' },
        //{ "height",     required_argument,      NULL,           'H' },
	{ "interval",   required_argument,      NULL,           'I' },
	{ "version",	no_argument,		NULL,		'v' },
        {"port",	required_argument,	NULL,	'P' },
	{ 0, 0, 0, 0 }
};

static void OptionsChosen()
{
        printf("---------- Options chosen -----------\n");
        printf("FPS : %d \n",fps);
        printf("Width : %d \n",width);
        printf("Height : %d \n",height);
        printf("Port : %d \n",port);
        if (!jpegFilename) {
             printf("No output for picture (server side) \n");
        }
        else {
           printf("Output of picture (server side) : %s \n",jpegFilename);
        }
        printf("JpegQuality : %d \n",jpegQuality);
        printf("Device name: %s \n",deviceName);

        }

int main(int argc, char **argv)
{


        for (;;) {
            int index, c = 0;

            c = getopt_long(argc, argv, short_options, long_options, &index);

            if (-1 == c)
                break;


            switch (c) {
                case 0: /* getopt_long() flag */
                    break;

            case 'P':
                // set width
                port = atoi(optarg);
                break;


                case 'd':
                    deviceName = optarg;
                    break;

                case 'h':
                    // print help
                    usage(stdout, argc, argv);
                    exit(EXIT_SUCCESS);

                case 'o':
                    // set jpeg filename
                    jpegFilename = optarg;
                    break;

                case 'q':
                    // set jpeg quality
                    jpegQuality = atoi(optarg);
                    break;

                case 'm':
    #ifdef IO_MMAP
                    io = IO_METHOD_MMAP;
    #else
                    fprintf(stderr, "You didn't compile for mmap support.\n");
                    exit(EXIT_FAILURE);
    #endif
                    break;

                case 'r':
    #ifdef IO_READ
                    io = IO_METHOD_READ;
    #else
                    fprintf(stderr, "You didn't compile for read support.\n");
                    exit(EXIT_FAILURE);
    #endif
                    break;

                case 'u':
    #ifdef IO_USERPTR
                    io = IO_METHOD_USERPTR;
    #else
                    fprintf(stderr, "You didn't compile for userptr support.\n");
                    exit(EXIT_FAILURE);
    #endif
                    break;

                case 'I':
                    // set fps
                    fps = atoi(optarg);
                    break;

                case 'v':
                    printf("Version: %s\n", VERSION);
                    exit(EXIT_SUCCESS);
                    break;

                default:
                    usage(stderr, argc, argv);
                    exit(EXIT_FAILURE);
            }
        }

        OptionsChosen();

        // open and initialize device
        deviceOpen();
        deviceInit();


        // start capturing
        captureStart();


	// process frames
        mainLoop(port);

	// stop capturing
        captureStop();

        // close device
        deviceUninit();
        deviceClose();

        if(jpegFilenamePart != 0){
                free(jpegFilename);
        }

        exit(EXIT_SUCCESS);

        return 0;
}
