#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "threadpool.h"
threadpool* create_threadpool(int );
void destroy_threadpool(threadpool* );

threadpool* create_threadpool(int num_threads_in_pool) {

	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL)) {
		fprintf(stderr, "the thread amount is not valid\n");
		return NULL;
	}

	threadpool* tpool = (threadpool*) malloc(sizeof(threadpool));
	if (tpool == NULL) {
		perror("error: cannot malloc threadpool\n");
		return NULL;
	}

	memset(tpool, 0, sizeof(threadpool));
	tpool->num_threads = num_threads_in_pool;

	if (pthread_mutex_init(&(tpool->qlock), 0)) {
		perror("error: cannot initializing the mutex\n");
	}
	if (pthread_cond_init(&(tpool->q_not_empty), 0)) {
		perror("error: cannot initializing 'cond q_not_empty'\n");
	}
	if (pthread_cond_init(&(tpool->q_empty), 0)) {
		perror("error: cannot initializing 'cond q_empty'\n");
	}
	// initializing the job of threads
	pthread_t* thread = (pthread_t*) malloc(sizeof(pthread_t)*tpool->num_threads);
	tpool->threads = thread;

	int i;
	for (i=0; i < tpool->num_threads; i++) {
		if (pthread_create(&(tpool->threads[i]), NULL, do_work, tpool) != 0) {
			perror("error: cannot create thread\n");
			printf("@ thread %d", i);
		}
	}
	return tpool;
}

//dispatches a task to queue
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg) {

	if ((from_me == NULL) || (dispatch_to_here == NULL)) {
		fprintf(stderr, "invalid parameters in dispatch function\n");
		return;
	}

	threadpool* tpool = (threadpool*) from_me;

	work_t* wrk = (work_t*) malloc(sizeof(work_t));
	memset(wrk, 0, sizeof(work_t));
	wrk->routine = dispatch_to_here;
	wrk->arg = arg;

	pthread_mutex_lock(&(tpool->qlock));
	if (tpool->dont_accept == 1) {
		printf("Dont_accept, cancel any dispatch\n");
		return;
	}
	if (tpool->qsize == 0) {
		tpool->qhead = wrk;
		tpool->qtail = wrk;
		pthread_cond_signal(&(tpool->q_not_empty));
	} else {
		tpool->qtail->next = wrk;
		tpool->qtail = wrk;
	}
	tpool->qsize++;
	pthread_mutex_unlock(&(tpool->qlock));
}

//this routine runs the delegate
void* do_work(void* p) {
	threadpool* tpool = (threadpool*) p;
	work_t* wrk = NULL;
	while (1)
	{
		pthread_mutex_lock(&(tpool->qlock));
		while (tpool->qsize < 1)
		{
			if (tpool->shutdown == 1) {
				pthread_mutex_unlock(&(tpool->qlock));
				pthread_exit(0);
			}
			pthread_mutex_unlock(&(tpool->qlock));
			pthread_cond_wait(&(tpool->q_not_empty), &(tpool->qlock));

			if (tpool->shutdown == 1) {
				pthread_mutex_unlock(&(tpool->qlock));
				pthread_exit(0);
			}
		}
		if (tpool->qsize < 1) {
			printf("queue size is 0, aborting\n");
			pthread_exit(0);
		}

		wrk = tpool->qhead;
		if (tpool->qsize) {
			tpool->qsize--;
		}
		if (tpool->qsize == 0) {
			tpool->qhead = NULL;
			tpool->qtail = NULL;
		} else {
			tpool->qhead = wrk->next;
		}

		if ((tpool->shutdown == 0) && (tpool->qsize == 0)) {
			pthread_cond_signal(&(tpool->q_empty));
		}
		pthread_mutex_unlock(&(tpool->qlock));

		(wrk->routine) (wrk->arg);
		wrk = NULL;
		free(wrk);
	}
}

//signal shutdown and destroy threadpool
void destroy_threadpool(threadpool* destroyme) {
	
	if (destroyme == NULL) {
		fprintf(stderr, "nothing to destroy, threadpool is null\n");
	}

	threadpool* tpool = (threadpool*) destroyme;

	pthread_mutex_lock(&(tpool->qlock));
	tpool->dont_accept = 1;
	
	while (tpool->qsize > 0) {
		pthread_cond_wait(&(tpool->q_empty), &(tpool->qlock));
	}
	
	tpool->shutdown = 1;
	pthread_cond_broadcast(&(tpool->q_not_empty));
	pthread_mutex_unlock(&(tpool->qlock));

	int i;
	for (i=0; i < tpool->num_threads; i++) {
		if (pthread_join(tpool->threads[i], NULL) != 0)
			perror("error in pthread_join\n");
	}

	pthread_mutex_destroy(&(tpool->qlock));
	pthread_cond_destroy(&(tpool->q_empty));
	pthread_cond_destroy(&(tpool->q_not_empty));

	free(tpool->threads);
	tpool->threads = NULL;
	free(tpool);
	tpool = NULL;
}