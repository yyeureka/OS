#include "gfserver.h"
#include "proxy-student.h"

#define BUFSIZE (630)

typedef struct MemoryStruct {
	char *data;
	size_t size;
} MemoryStruct;

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
	size_t bytes;
	MemoryStruct *mem = (MemoryStruct *)userp;
	char *tmp;

	bytes = size * nmemb;

	tmp = realloc(mem->data, mem->size + bytes + 1);
	if (NULL == tmp) {
		/* out of memory */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->data = tmp;
	memcpy(&(mem->data[mem->size]), data, bytes);
	mem->size += bytes;
	mem->data[mem->size] = 0;

	return bytes;
}

/*
 * Replace with an implementation of handle_with_curl and any other
 * functions you may need.
 */
ssize_t handle_with_curl(char *url, MemoryStruct *chunk){
	CURL *curl_handle;
	CURLcode res;	
	curl_off_t file_len = 0;
 
	curl_handle = curl_easy_init();
	if (NULL == curl_handle) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}
 
	res = curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	if (CURLE_OK != res) {
		fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		return -1;
	}

	/* specify function */ 
	res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb);
	if (CURLE_OK != res) {
		fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		return -1;
	}

	/* specify memory space */ 
	res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
	if (CURLE_OK != res) {
		fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		return -1;
	}

	res = curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	if (CURLE_OK != res) {
		fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		return -1;
	}

	chunk->data = malloc(1); // '\0'
	chunk->size = 0;

	res = curl_easy_perform(curl_handle);
	if (CURLE_OK != res) {
		curl_easy_cleanup(curl_handle);

		if (CURLE_HTTP_RETURNED_ERROR == res) {
			fprintf(stdout, "file not found: %s\n", curl_easy_strerror(res));
			return 0;
		}

		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		return -1;
	}

	/* get the number of downloaded bytes */ 
    res = curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD_T, &file_len);
	if (CURLE_OK != res) {
		fprintf(stderr, "curl_easy_getinfo() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl_handle);
		return -1;
	}
	if (chunk->size != file_len) {
		fprintf(stderr, "receive incorrect file length\n");
		curl_easy_cleanup(curl_handle);
		return -1;
	}
	
	curl_easy_cleanup(curl_handle);

	return file_len;
}

/*
 * We provide a dummy version of handle_with_file that invokes handle_with_curl
 * as a convenience for linking.  We recommend you simply modify the proxy to
 * call handle_with_curl directly.
 */
ssize_t handle_with_file(gfcontext_t *ctx, const char *path, void* arg){
	char url[BUFSIZE];
	MemoryStruct chunk;
	ssize_t file_len = -1;
	ssize_t bytes_total = 0;
	ssize_t bytes = 0;

	/* specify URL */ 
	strncpy(url, (char *)arg, BUFSIZE);
	strncat(url, path, BUFSIZE);
	// printf("%s\n", url);

	file_len = handle_with_curl(url, &chunk);

	if (-1 == file_len) {
		fprintf(stderr, "handle_with_curl() failed\n");
		gfs_sendheader(ctx, GF_ERROR, 0);
	}
	else if (0 == file_len) {
		gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}
	else {
		gfs_sendheader(ctx, GF_OK, file_len);

		bytes_total = 0;
		while (bytes_total < file_len) {
			bytes = gfs_send(ctx, &(chunk.data[bytes_total]), file_len - bytes_total);
			bytes_total += bytes;
		}
	}

	free(chunk.data);

	return file_len;
}	
