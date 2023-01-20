#include "gfserver.h"
#include "cache-student.h"

ssize_t handle_with_cache(gfcontext_t *ctx, const char *path, void *arg){
	char *shm_name;
	int shm_fd = -1;
	char *ptr = MAP_FAILED;
	char sem_name_r[SEMNAMESIZE];
	char sem_name_w[SEMNAMESIZE];
	sem_t *sem_r = SEM_FAILED;
	sem_t *sem_w = SEM_FAILED;
	msg_t msg;

	ssize_t file_len = SERVER_FAILURE;
	ssize_t read_bytes;
	ssize_t write_bytes;
	ssize_t bytes_total = 0;
	ssize_t bytes = 0;

	if (strlen(path) > PATHSIZE) {
		fprintf(stderr, "ERROR path overflow\n");
	}

	pthread_mutex_lock(&queue_lock);
		while (steque_isempty(shm_pool.segment_q)) {
			pthread_cond_wait(&dequeue_phase, &queue_lock);
		}

		shm_name = (char *)steque_pop(shm_pool.segment_q);
	pthread_mutex_unlock(&queue_lock);

	// Open shared memory
	shm_fd = shm_open(shm_name, O_RDONLY, 0);
	if (0 > shm_fd) {
		fprintf(stderr, "%s open failed\n", shm_name);
		goto cleanup;
	}

	// Map shared memory
	ptr = mmap(NULL, shm_pool.segsize, PROT_READ, MAP_SHARED, shm_fd, 0);
	if (MAP_FAILED == ptr) {
		fprintf(stderr, "%s mmap() failed\n", shm_name);
		goto cleanup;
	}

	// Create semaphores
	sprintf(sem_name_r, "%s%s", shm_name, "r");
	sem_r = sem_open(sem_name_r, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
	if (SEM_FAILED == sem_r) {
		fprintf(stderr, "%s create fail\n", sem_name_r);
		goto cleanup;
	}

	sprintf(sem_name_w, "%s%s", shm_name, "w");
	sem_w = sem_open(sem_name_w, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
	if (SEM_FAILED == sem_w) {
		fprintf(stderr, "%s create fail\n", sem_name_w);
		goto cleanup;
	}

	// Send request to cache
	memset(&msg, 0, sizeof(msg));
	msg.segsize = shm_pool.segsize;
	memcpy(msg.path, path, strlen(path));
	memcpy(msg.shm_name, shm_name, strlen(shm_name));
	memcpy(msg.sem_name_r, sem_name_r, strlen(sem_name_r));
	memcpy(msg.sem_name_w, sem_name_w, strlen(sem_name_w));

	if (0 > mq_send(shm_pool.msgid, (const char *)&msg, MSGSIZE, 0)) {
		fprintf(stderr, "%s mq_send() failed\n", shm_name);
		goto cleanup;
	}
	
	// Read file length from shared memory and send it to client
	if (0 > sem_wait(sem_r)) {
		fprintf(stderr, "%s sem_wait() failed\n", sem_name_r);
		goto cleanup;
	}

	file_len = atoi(ptr);
	fprintf(stdout, "len:%ld%s%s\n", file_len, msg.path, shm_name);
	if (0 == file_len) {
		// fprintf(stdout, "file not found\n");
		gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
		goto cleanup;
	}
	gfs_sendheader(ctx, GF_OK, file_len);
	
	if (0 > sem_post(sem_w)) {
		fprintf(stderr, "%s sem_post() failed\n", sem_name_w);
		goto cleanup;
	}

	/* Read file from shared memory and send it to client. */
	bytes_total = 0;
	while (bytes_total < file_len) {
		if (0 > sem_wait(sem_r)) {
			fprintf(stderr, "%s sem_wait() failed\n", sem_name_r);
			break;
		}

		read_bytes = MIN(file_len - bytes_total, shm_pool.segsize);
		write_bytes = 0;
		// printf("read:%ld,%ld,%ld\n", read_bytes, bytes_total, file_len);

		while (write_bytes < read_bytes) {
			bytes = gfs_send(ctx, &(ptr[write_bytes]), read_bytes - write_bytes);
			if (0 >= bytes) {
				fprintf(stderr, "%s gfs_send() failed\n", shm_name);
				goto cleanup;
			}

			write_bytes += bytes;
			bytes_total += bytes;
		}

		if (0 > sem_post(sem_w)) {
			fprintf(stderr, "%s sem_post() failed\n", sem_name_w);
			break;
		}
		//printf("write:%ld,%ld,%ld\n", read_bytes, bytes_total, file_len);
	}

cleanup:
	if ((MAP_FAILED != ptr) && (0 > munmap(ptr, shm_pool.segsize))) {
		fprintf(stderr, "%s munmap() failed\n", shm_name);
	}

	if ((shm_fd > 0) && (0 > close(shm_fd))) {
		fprintf(stderr, "%s close() failed\n", shm_name);
	}

	if (SEM_FAILED != sem_r) {
		if (0 > sem_close(sem_r)) {
    		fprintf(stderr, "%s sem_close() fail\n", sem_name_r);
		}
		if (0 > sem_unlink(sem_name_r)) {
			fprintf(stderr, "%s sem_unlink() fail\n", sem_name_r);
		}
	}
	if (SEM_FAILED != sem_w) {
		if (0 > sem_close(sem_w)) {
    		fprintf(stderr, "%s sem_close() fail\n", sem_name_w);
		}
		if (0 > sem_unlink(sem_name_w)) {
			fprintf(stderr, "%s sem_unlink() fail\n", sem_name_w);
		}
	}

	pthread_mutex_lock(&queue_lock);
		steque_enqueue(shm_pool.segment_q, shm_name);
		pthread_cond_signal(&dequeue_phase);
	pthread_mutex_unlock(&queue_lock);

	return file_len;
}

