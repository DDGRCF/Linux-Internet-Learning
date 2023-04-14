#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define perr_cmd(s, num, command)                                 \
  if (num == -1) {                                                \
    num = errno;                                                  \
  }                                                               \
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num)); \
  command

#define perr_exit(s, num) perr_cmd(s, num, exit(num))

typedef struct {
  void (*startfunc)(void*);
  void* arg;
} pthreadpool_task_t;

typedef struct {
  pthread_mutex_t pthlock;
  pthread_cond_t pthcond;
  pthread_t* threads;
  pthreadpool_task_t* task_queue;
  int queue_size;
  int count;
  int head_index;
  int tail_index;

  int thread_count;
  bool shutdown;

  int started;
} threadpool_t;

threadpool_t* pthreadpool_create(int thread_count, int queue_size, int flags) {
  int ret;
  auto pth_pool = new threadpool_t;
  // pth_pool->pthlock = PTHREAD_MUTEX_INITIALIZER;
  // pth_pool->pthcond = PTHREAD_COND_INITIALIZER;
  pth_pool->count = 0;
  pth_pool->threads = new pthread_t[thread_count];
  pth_pool->task_queue = new pthreadpool_task_t[queue_size];
  pth_pool->queue_size = queue_size;
  pth_pool->head_index = pth_pool->tail_index = 0;
  pth_pool->thread_count = 0;
  pth_pool->shutdown = false;
  pth_pool->started = 0;

  ret = pthread_mutex_init(&pth_pool->pthlock, nullptr);
  if (ret != 0) {
    perr_cmd("pthread_mutex_init", ret, return nullptr);
  }
  ret = pthread_cond_init(&pth_pool->pthcond, nullptr);
  if (ret != 0) {
    perr_cmd("pthread_cond_init", ret, return nullptr);
  }

  if (pth_pool->threads == nullptr || pth_pool->task_queue == nullptr) {
    pthreadpool_free(pth_pool, 0);
  }
  pthread_attr_t pth_attr;
  ret = pthread_attr_init(&pth_attr);
  if (ret != 0) {
    perr_cmd("pthread_attr_init", ret, return nullptr);
  }
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  for (u_int32_t i = 0; i < pth_pool->thread_count; i++) {
    ret = pthread_create(&pth_pool->threads[i], &pth_attr, pthreadpool_worker,
                         &pth_pool);
    if (ret != 0) {
      perr_cmd("pthread_create", ret, return nullptr);
    }
  }
}

void* pthreadpool_worker(void* dst) {}

int pthreadpool_destroy(threadpool_t* pool, int flags) {
  int ret;
  if (pool == nullptr) {
    return -1;
  }
  ret = pthread_mutex_lock(&pool->pthlock);
  if (ret != 0) {
    perr_cmd("pthread_mutex_lock", ret, return -1);
  }
  ret = pthread_cond_broadcast(&pool->pthcond);
  if (ret != 0) {
    perr_cmd("pthread_cond_broacast", ret, return -1);
  }
  ret = pthread_mutex_unlock(&pool->pthlock);
  if (ret != 0) {
    perr_cmd("pthread_mutex_unlock", ret, return -1);
  }
  ret = pthreadpool_free(pool, flags);
  if (ret != 0) {
    perr_cmd("pthreadpool_free", ret, return -1);
  }
  return 0;
}

int pthreadpool_free(threadpool_t* pool, int flags) {
  int ret = 0;
  ret = pthread_mutex_destroy(&pool->pthlock);
  if (ret != 0) {
    perr_cmd("pthread_mutex_destroy", ret, return -1);
  }
  ret = pthread_cond_destroy(&pool->pthcond);
  if (ret != 0) {
    perr_cmd("pthread_cond_destory", ret, return -1);
  }
  if (pool->threads != nullptr) {
    delete[] pool->threads;
  }
  pool->thread_count = 0;
  if (pool->task_queue != nullptr) {
    delete[] pool->task_queue;
  }
  pool->count = 0;
  pool->shutdown = true;
  pool->started = 0;
  pool->head_index = pool->tail_index = 0;
  delete pool;
}