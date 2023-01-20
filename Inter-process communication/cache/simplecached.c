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
#include <curl/curl.h>

#include "gfserver.h"
#include "cache-student.h"
#include "shm_channel.h"
#include "simplecache.h"

#if !defined(CACHE_FAILURE)
#define CACHE_FAILURE (-1)
#endif // CACHE_FAILURE

#define MAX_CACHE_REQUEST_LEN 5041

unsigned long int cache_delay;
int nthreads = 7;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dequeue_phase = PTHREAD_COND_INITIALIZER;
pthread_t *worker_threads;
steque_t *request_queue;
mqd_t msgid = -1;

static void _sig_handler(int signo){
	if (signo == SIGTERM || signo == SIGINT){
		// Cleanup thread and request queue	
		cleanup_threads(nthreads);

		// Detroy cache
		simplecache_destroy();

		// Cleanup IPCs
		if (0 > mq_close(msgid)) {
			fprintf(stderr, "mq_close() failed\n");
		}
		if (0 > mq_unlink(MQNAME)) {
			fprintf(stderr, "mq_unlink() failed\n");
		}

		exit(signo);
	}
}

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  simplecached [options]\n"                                                  \
"options:\n"                                                                  \
"  -c [cachedir]       Path to static files (Default: ./)\n"                  \
"  -t [thread_count]   Thread count for work queue (Default is 7, Range is 1-31415)\n"      \
"  -d [delay]          Delay in simplecache_get (Default is 0, Range is 0-5000000 (microseconds)\n "	\
"  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"cachedir",           required_argument,      NULL,           'c'},
  {"nthreads",           required_argument,      NULL,           't'},
  {"help",               no_argument,            NULL,           'h'},
  {"hidden",			 no_argument,			 NULL,			 'i'}, /* server side */
  {"delay", 			 required_argument,		 NULL, 			 'd'}, // delay.
  {NULL,                 0,                      NULL,             0}
};

void Usage() {
  fprintf(stdout, "%s", USAGE);
}

int main(int argc, char **argv) {
	char *cachedir = "locals.txt";
	char option_char;
	struct mq_attr attr;
	msg_t msg;
	msg_t *cur;

	/* disable buffering to stdout */
	setbuf(stdout, NULL);

	while ((option_char = getopt_long(argc, argv, "id:c:hlxt:", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			default:
				Usage();
				exit(1);
			case 'c': //cache directory
				cachedir = optarg;
				break;
			case 'h': // help
				Usage();
				exit(0);
				break;    
			case 't': // thread-count
				nthreads = atoi(optarg);
				break;
			case 'd':
				cache_delay = (unsigned long int) atoi(optarg);
				break;
			case 'i': // server side usage
			case 'l': // experimental
			case 'x': // experimental
				break;
		}
	}

	if (cache_delay > 5000000) {
		fprintf(stderr, "Cache delay must be less than 5000000 (us)\n");
		exit(__LINE__);
	}

	if ((nthreads>31415) || (nthreads < 1)) {
		fprintf(stderr, "Invalid number of threads\n");
		exit(__LINE__);
	}

	if (SIG_ERR == signal(SIGINT, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGINT...exiting.\n");
		exit(CACHE_FAILURE);
	}

	if (SIG_ERR == signal(SIGTERM, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGTERM...exiting.\n");
		exit(CACHE_FAILURE);
	}

	// Create message queue
	// attr.mq_flags = 0; // Block mode
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSGSIZE;
	// attr.mq_curmsgs = 0;
	msgid = mq_open(MQNAME, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, &attr);
	if (msgid < 0) {
		fprintf(stderr, "mq_open() failed\n");
		exit(__LINE__);
	}

	// Initialize cache
	simplecache_init(cachedir);

	// Initialie thread and request queue
	init_threads(nthreads);

	// receive request, enqueue
	while (1) {
		memset(&msg, 0, sizeof(msg));
		if (0 > mq_receive(msgid, (char *)&msg, MSGSIZE, NULL)) {
			fprintf(stderr, "mq_receive() failed\n");
			break;
		}
		cur = (msg_t *)malloc(sizeof(msg_t));
		memcpy(cur, &msg, sizeof(msg_t));
		fprintf(stdout, "rcv:%s%s\n", cur->path, cur->shm_name);

		pthread_mutex_lock(&queue_lock);
			// Enqueue
			steque_enqueue(request_queue, cur);
			pthread_cond_signal(&dequeue_phase);
		pthread_mutex_unlock(&queue_lock);
	}

	return 0;
}

void init_threads(int numthreads) {
	int i;

	request_queue = (steque_t *)malloc(sizeof(steque_t));
	steque_init(request_queue);

	worker_threads = (pthread_t *)malloc(numthreads * sizeof(pthread_t));
	for (i = 0; i < numthreads; i++) {
		pthread_create(&(worker_threads[i]), NULL, handle_request, NULL);
	}
}

void cleanup_threads(int numthreads) {
	int i;
	msg_t stop_signal;

	stop_signal.segsize = 0;
	pthread_mutex_lock(&queue_lock);
		steque_enqueue(request_queue, &stop_signal);
		pthread_cond_signal(&dequeue_phase);
	pthread_mutex_unlock(&queue_lock);

	for(i = 0; i < numthreads; i++) {
		pthread_join(worker_threads[i], NULL);
	}
	free(worker_threads);

	steque_destroy(request_queue);
	free(request_queue);
}

void *handle_request(void *worker_id) {
	msg_t *msg;
	int shm_fd;
	char *ptr;
	sem_t *sem_r;
	sem_t *sem_w;
	
	int fildes;
	struct stat statbuf;
	size_t file_len; 
	ssize_t read_bytes;
	ssize_t write_bytes;
	ssize_t bytes_total;
	ssize_t bytes;

	while (1) {
		shm_fd = -1;
		ptr = MAP_FAILED;
		sem_r = SEM_FAILED;
		sem_w = SEM_FAILED;

		pthread_mutex_lock(&queue_lock);
			while (steque_isempty(request_queue)) {
				pthread_cond_wait(&dequeue_phase, &queue_lock);
			}

			msg = (msg_t *)steque_front(request_queue);
			if (0 == msg->segsize) { // Stop signal
				pthread_cond_signal(&dequeue_phase);
			}
			else {
				steque_pop(request_queue);
			}
		pthread_mutex_unlock(&queue_lock);

		if (0 == msg->segsize) { // Stop signal
			break;
		}
		
		// Open shared memory
		while (0 > (shm_fd = shm_open(msg->shm_name, O_RDWR, 0))) {
			fprintf(stdout, "%s open wait\n", msg->shm_name);
			sleep(1);
		}

		// Map shared memory
		ptr = mmap(NULL, msg->segsize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (MAP_FAILED == ptr) {
			fprintf(stderr, "%s mmap() failed\n", msg->shm_name);
			goto cleanup;
		}

		// Open semaphores
		while (SEM_FAILED == (sem_r = sem_open(msg->sem_name_r, O_RDWR))) {
			fprintf(stdout, "%s open wait\n", msg->sem_name_r);
			sleep(1);
		}
		while (SEM_FAILED == (sem_w = sem_open(msg->sem_name_w, O_RDWR))) {
			fprintf(stdout, "%s open wait\n", msg->sem_name_w);
			sleep(1);
		}

		// Find requested file
		fildes = simplecache_get(msg->path);
		if (-1 == fildes) {
			// fprintf(stdout, "file not found\n");
			file_len = 0;
		}
		else {
			if (0 > fstat(fildes, &statbuf)) {
				fprintf(stderr, "fstat() failed%s\n", msg->path);
				file_len = 0;
			}
			else {
				file_len = (size_t)statbuf.st_size;
			}
		}

		// Write file length into shared memory
		memset(ptr, 0, msg->segsize);
		sprintf(ptr, "%ld", file_len); 
		if (0 > sem_post(sem_r)) {
            fprintf(stderr, "%s sem_post() failed\n", msg->sem_name_r);
        }

		// Write file into shared memory
		bytes_total = 0;
		while (bytes_total < file_len) {
			if (0 > sem_wait(sem_w)) {
				fprintf(stderr, "%s sem_wait() failed\n", msg->sem_name_w);
				break;
			}

			read_bytes = MIN(file_len - bytes_total, msg->segsize);
			write_bytes = 0;
			// printf("read:%ld,%ld,%lu\n", read_bytes, bytes_total, file_len);

			while (write_bytes < read_bytes) {
				bytes = pread(fildes, &(ptr[write_bytes]), read_bytes - write_bytes, bytes_total);
				if (bytes <= 0){
					fprintf(stderr, "read error, %ld, %ld, %ld, %ld, %lu\n", bytes, write_bytes, read_bytes, bytes_total, file_len);
					goto cleanup;
				}
				
				write_bytes += bytes;
				bytes_total += bytes;
			}

			if (0 > sem_post(sem_r)) {
				fprintf(stderr, "%s sem_post() failed\n", msg->sem_name_r);
				break;
			}
			// printf("write:%ld,%ld,%lu\n", write_bytes, bytes_total, file_len);
		}

cleanup:
		if ((MAP_FAILED != ptr) && (0 > munmap(ptr, msg->segsize))) {
			fprintf(stderr, "%s munmap() failed\n", msg->shm_name);
		}

		if ((shm_fd > 0) && (0 > close(shm_fd))) {
			fprintf(stderr, "%s close() failed\n", msg->shm_name);
		}

		if ((SEM_FAILED != sem_r) && (0 > sem_close(sem_r))) {
			fprintf(stderr, "%s sem_close() failed\n", msg->sem_name_r);
		}
		if ((SEM_FAILED != sem_w) && (0 > sem_close(sem_w))) {
			fprintf(stderr, "%s sem_close() failed\n", msg->sem_name_w);
		}	

		free(msg);	
	}

	pthread_exit(0);
}