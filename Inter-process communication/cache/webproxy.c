#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <printf.h>

#include "gfserver.h"
#include "cache-student.h"

/* note that the -n and -z parameters are NOT used for Part 1 */
/* they are only used for Part 2 */                         
#define USAGE                                                                         \
"usage:\n"                                                                            \
"  webproxy [options]\n"                                                              \
"options:\n"                                                                          \
"  -n [segment_count]  Number of segments to use (Default: 6)\n"                      \
"  -p [listen_port]    Listen port (Default: 30605)\n"                                 \
"  -t [thread_count]   Num worker threads (Default: 12, Range: 1-520)\n"              \
"  -s [server]         The server to connect to (Default: GitHub test data)\n"     \
"  -z [segment_size]   The segment size (in bytes, Default: 6200).\n"                  \
"  -h                  Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"segment-count", required_argument,      NULL,           'n'},
  {"port",          required_argument,      NULL,           'p'},
  {"thread-count",  required_argument,      NULL,           't'},
  {"server",        required_argument,      NULL,           's'},
  {"segment-size",  required_argument,      NULL,           'z'},         
  {"help",          no_argument,            NULL,           'h'},
  {"hidden",        no_argument,            NULL,           'i'}, /* server side */
  {NULL,            0,                      NULL,            0}
};

extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg);

static gfserver_t gfs;
shm_pool_t shm_pool;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dequeue_phase = PTHREAD_COND_INITIALIZER;

static void _sig_handler(int signo){
  if (signo == SIGTERM || signo == SIGINT){
    cleanup_ipcs();
    gfserver_stop(&gfs);

    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int i;
  int option_char = 0;
  unsigned short port = 30605;
  unsigned short nworkerthreads = 12;
  unsigned int nsegments = 6;
  size_t segsize = 6200;
  char *server = "https://raw.githubusercontent.com/gt-cs6200/image_data";

  /* disable buffering on stdout so it prints immediately */
  setbuf(stdout, NULL);

  if (signal(SIGINT, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  /* Parse and set command line arguments */
  while ((option_char = getopt_long(argc, argv, "s:qt:hn:xp:z:l", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(__LINE__);
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 'n': // segment count
        nsegments = atoi(optarg);
        break;   
      case 's': // file-path
        server = optarg;
        break;                                          
      case 'z': // segment size
        segsize = atoi(optarg);
        break;
      case 't': // thread-count
        nworkerthreads = atoi(optarg);
        break;
      case 'i':
      case 'x':
      case 'l':
        break;
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }

  if (segsize < 128) {
    fprintf(stderr, "Invalid segment size\n");
    exit(__LINE__);
  }

  if (!server) {
    fprintf(stderr, "Invalid (null) server name\n");
    exit(__LINE__);
  }

  if (port < 1024) {
    fprintf(stderr, "Invalid port number\n");
    exit(__LINE__);
  }

  if (nsegments < 1) {
    fprintf(stderr, "Must have a positive number of segments\n");
    exit(__LINE__);
  }

  if ((nworkerthreads < 1) || (nworkerthreads > 520)) {
    fprintf(stderr, "Invalid number of worker threads\n");
    exit(__LINE__);
  }

  // Initialize shared memory set-up here
  init_ipcs(nsegments, segsize);

  // Initialize server structure here
  gfserver_init(&gfs, nworkerthreads);

  // Set server options here
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 121);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_cache);
  gfserver_setopt(&gfs, GFS_PORT, port);

  // Set up arguments for worker here
  for(i = 0; i < nworkerthreads; i++) {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, "data");
  }
  
  // Invoke the framework - this is an infinite loop and shouldn't return
  gfserver_serve(&gfs);

  // not reached
  return -1;
}

void init_ipcs(unsigned int nsegments, size_t segsize) {
  int i;
  int shm_fd = -1;
  char buffer[10] = "";
  char *shm_name;

  shm_pool.nsegments = nsegments;
  shm_pool.segsize = segsize;
  shm_pool.segment_q = (steque_t *)malloc(sizeof(steque_t));
  steque_init(shm_pool.segment_q);  

  // Create n shared memory segments
  for (i = 0; i < nsegments; i++) {
    if (i >= 10000) {
      fprintf(stderr, "shm_name overflow\n");
    }

    sprintf(buffer, "%s%d", "/", i);
    shm_name = strdup(buffer);

    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
		if (0 > shm_fd) {
			fprintf(stderr, "%s create failed\n", shm_name);
		}

		if (0 > ftruncate(shm_fd, shm_pool.segsize)) {
			fprintf(stderr, "%s ftruncate() failed\n", shm_name);
		}

    pthread_mutex_lock(&queue_lock);
			// Enqueue
      steque_enqueue(shm_pool.segment_q, shm_name);
      // fprintf(stdout, "create a shm:%d%s\n", shm_pool.segment_q->N, segment->shm_name);
      pthread_cond_signal(&dequeue_phase);
		pthread_mutex_unlock(&queue_lock);
  }

  // Open message queue
  while (0 > (shm_pool.msgid = mq_open(MQNAME, O_WRONLY))) {
    fprintf(stdout, "mq_open() wait\n");
    sleep(1); 
  }
}

void cleanup_ipcs() {
  char *shm_name;

  pthread_mutex_lock(&queue_lock);
		while (shm_pool.segment_q->N != shm_pool.nsegments) {
			pthread_cond_wait(&dequeue_phase, &queue_lock);
		}
	pthread_mutex_unlock(&queue_lock);

  while (!steque_isempty(shm_pool.segment_q)) {
    shm_name = (char *)steque_pop(shm_pool.segment_q);
    // fprintf(stdout, "delete a shm:%d%s\n", shm_pool.segment_q->N, shm_name);

    if (0 > shm_unlink(shm_name)) {
      fprintf(stderr, "%s shm_unlink() fail\n", shm_name);
    }

    free(shm_name);
    shm_name = NULL;
  }

  if ((shm_pool.msgid > 0) && (0 > mq_close(shm_pool.msgid))) {
    fprintf(stderr, "mq_close() fail\n");
  }
}
