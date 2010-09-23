#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>      /* for videodev2.h */
#include <sys/stat.h>
#include <linux/videodev2.h>

#include "spike_V4Lcapture.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define N_VID_BUFFERS 4
#define LINE_SIZE_MAX     1024

struct buffer {
    void *          data;
    size_t          length;
};

// module vars - poor man's class vars
struct buffer x_video_buf[N_VID_BUFFERS];
int x_video_fd  = -1; 
int x_imagesize = 0; 
int x_width = 0;
int x_height = 0;

unsigned char x_line_bufU[LINE_SIZE_MAX];
unsigned char x_line_bufV[LINE_SIZE_MAX];

static int xioctl (int fd, int request, void *arg)
{
    int r;

    do {
	r = ioctl (fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

static void errno_exit(const char *s)
{
    fprintf (stderr, "V4L: %s error %d, %s\n",
	    s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}

static void init_mmap ()
{
    int iBuffer;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req;
    

    CLEAR (req);

    req.count       = N_VID_BUFFERS;
    req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory      = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(x_video_fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf (stderr, "V4L device does not support memory mapping\n");
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    if (req.count < N_VID_BUFFERS) {
        fprintf (stderr, "V4L: Insufficient buffer memory on V4L device s\n");
        exit (EXIT_FAILURE);
    }

    for (iBuffer = 0; iBuffer < req.count; ++iBuffer) {

        CLEAR (buf);

        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = iBuffer;

        if (-1 == xioctl(x_video_fd, VIDIOC_QUERYBUF, &buf))
            errno_exit ("VIDIOC_QUERYBUF");

        x_video_buf[iBuffer].length = buf.length;
        x_video_buf[iBuffer].data =
	      mmap (NULL /* start anywhere */, buf.length,
		    PROT_READ | PROT_WRITE /* required */,
		    MAP_SHARED /* recommended */,
		    x_video_fd, buf.m.offset);

        if (MAP_FAILED == x_video_buf[iBuffer].data)
            errno_exit ("mmap");
    }
}

// global functions start here
 
void open_device(const char * video_dev_name, int * pvideo_fd)
{
    struct stat st; 

    if (-1 == stat (video_dev_name, &st)) {
        fprintf (stderr, "V4L: Cannot identify '%s': %d, %s\n",
             video_dev_name, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (!S_ISCHR (st.st_mode)) {
        fprintf (stderr, "V4L: %s is no device\n", video_dev_name);
        exit (EXIT_FAILURE);
    }

    x_video_fd = open (video_dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (x_video_fd == -1) {
        fprintf (stderr, "V4L: Cannot open '%s': %d, %s\n",
             video_dev_name, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }
    * pvideo_fd = x_video_fd;
    fprintf (stderr, "V4L: Opened video device: %s\n",video_dev_name );
}


void init_device( int inputnum, int width, int height )
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    v4l2_std_id standard_id;
    struct v4l2_input input;
    unsigned int min;

    x_height = height;
    x_width = width;
    x_imagesize = width *  height * BYTES_PER_PIXEL;
    min = 0;
    if (xioctl (x_video_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if (EINVAL == errno) {
            fprintf (stderr, "V4L: opened device is no V4L2 device\n");
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf (stderr, "V4L: opened device is no video capture device\n");
        exit (EXIT_FAILURE);
    }
    /* SR Don't need this? */
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
	fprintf (stderr, "V4L: opened device does not support streaming i/o\n");
	exit (EXIT_FAILURE);
    }
    /* Select video input, video standard and tune here. */

    /* Set the video standard to NTSC */
    standard_id = V4L2_STD_NTSC;
    if (xioctl (x_video_fd, VIDIOC_S_STD, &standard_id) == -1) {
	fprintf(stderr, "V4L: Error setting video standard to NTSC\n");
    }

    /* Set the video input to composite if it is not specified*/
    CLEAR(input);
    input.index = 0;
    if (inputnum != -1) {
	input.index = inputnum;
    }
    while (1) {
	if (xioctl (x_video_fd, VIDIOC_ENUMINPUT, &input) == -1) {
	    fprintf(stderr, "V4L: Error querying input %d\n", input.index);
	    exit(-1);
	}
	/* check to see if this is a camera input */
	if (input.type == V4L2_INPUT_TYPE_CAMERA) {
	    xioctl(x_video_fd, VIDIOC_S_INPUT, &input);
	    fprintf(stderr, "V4L: Using video input %s\n", input.name);
	    break;
	}
	input.index++;
    }


    CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(x_video_fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(x_video_fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
		fprintf(stderr, "V4L: Set cropping failed - invalid value: %d,%d;  %d x %d pixels.\n", 
                                                crop.c.left, crop.c.top, crop.c.width, crop.c.height);
                break;
            default:
                /* Errors ignored. */
		fprintf(stderr, "V4L: Set cropping failed.\n");
                break;
            }
        }
        else {
	    fprintf(stderr, "V4L: Cropping set to %d,%d; %d x %d pixels.\n",
                            crop.c.left, crop.c.top, crop.c.width, crop.c.height);
        }
    } 
    else {	
      /* Errors ignored. */
      fprintf(stderr, "V4L: Cropping not supported.\n");
    }


    CLEAR (fmt);

    fmt.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width; 
    fmt.fmt.pix.height      = height;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    fmt.fmt.pix.field       = V4L2_FIELD_TOP;

    if (-1 == xioctl(x_video_fd, VIDIOC_S_FMT, &fmt))
        errno_exit ("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
	    fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
	    fmt.fmt.pix.sizeimage = min;

    init_mmap ();

}



void start_capturing ()
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < N_VID_BUFFERS; ++i) {
	struct v4l2_buffer buf;

	CLEAR (buf);

	buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory  = V4L2_MEMORY_MMAP;
	buf.index   = i;

	if (-1 == xioctl(x_video_fd, VIDIOC_QBUF, &buf))
	    errno_exit ("VIDIOC_QBUF");
    }

    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(x_video_fd, VIDIOC_STREAMON, &type))
	errno_exit ("VIDIOC_STREAMON");
    fprintf(stderr, "V4L: started capturing\n");
}

int read_frame( unsigned char *outbuf, long int * sec, long int * usec )
{
    struct v4l2_buffer buf;

    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(x_video_fd, VIDIOC_DQBUF, &buf)) {
	switch (errno) {
	    case EAGAIN:
//	      fprintf(stderr, "V4L: read_frame EAGAIN error\n"); // we should never get here if select works
		return 0;
	    case EIO:
		/* Could ignore EIO, see spec. */
		/* fall through */
	    default:
		errno_exit ("VIDIOC_DQBUF");
	}
    }

    /* copy the data from the current buffer to the output
     * buffer */
    memcpy((char *) outbuf, (char *) x_video_buf[buf.index].data, x_imagesize);

    // return the timestamp in sec/microsec
    *sec = buf.timestamp.tv_sec;
    *usec = buf.timestamp.tv_usec;
    

    if (-1 == xioctl(x_video_fd, VIDIOC_QBUF, &buf))
	    errno_exit ("VIDIOC_QBUF");
    
    return 1;
}

void stop_capturing()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(x_video_fd, VIDIOC_STREAMOFF, &type))
	    errno_exit ("VIDIOC_STREAMOFF");
    fprintf(stderr, "V4L: stopped capturing\n");

}

void uninit_device()
{
    unsigned int i;
    for (i = 0; i < N_VID_BUFFERS; ++i) {
	if (-1 == munmap (x_video_buf[i].data, x_video_buf[i].length)) {
	    errno_exit ("munmap");
	}
    }
}

void close_device()
{
    if (-1 == close (x_video_fd))
	errno_exit ("close");

    x_video_fd = -1;
}

void YUY422to422P( unsigned char *inbuf, 
                    unsigned char *outY, unsigned char *outU, unsigned char *outV )
{
  int iQuad, nQuad;

  nQuad = x_imagesize / 4;
  for( iQuad = 0; iQuad < nQuad; iQuad++){
    *outY++ = *inbuf++;
    *outU++ = *inbuf++;
    *outY++ = *inbuf++;
    *outV++ = *inbuf++;
  }

}


void YUY422to420P( unsigned char *inbuf, 
                    unsigned char *outY, unsigned char *outU, unsigned char *outV )
{
  int iPix, nPix, iLine, nLines;
  int iTmp;
  unsigned char * pLineU;
  unsigned char * pLineV;
  
  nPix = x_width / 2;
  nLines = x_height /2;  // these are actually double lines

  for( iLine = 0; iLine < nLines; iLine++){
    pLineU = x_line_bufU;
    pLineV = x_line_bufV;
    for(iPix = 0; iPix < nPix; iPix++) {
      *outY++ = *inbuf++;
      *pLineU++ = *inbuf++;  // put chroma into the line buffer
      *outY++ = *inbuf++;
      *pLineV++ = *inbuf++;  // put chroma into the line buffer
    }

    pLineU = x_line_bufU;
    pLineV = x_line_bufV;
    for(iPix = 0; iPix < nPix; iPix++) {
      *outY++ = *inbuf++;
      iTmp = *inbuf++  + *pLineU++;// add buffered value
      *outU++ = iTmp/2;
      *outY++ = *inbuf++;
      iTmp = *inbuf++  + *pLineV++; // add buffered value
      *outV++ = iTmp/2;
    }
  }

}






