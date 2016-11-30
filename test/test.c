#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "stm.h"

#define ARRAY_SZ 10000

typedef struct {
  int thread_id;
  int num_threads;
  Word *array;
} arg_t;

void *InterferingWrites(void *arg) {
  arg_t *d = (arg_t *)arg;
  int me = d->thread_id;
  int nthreads = d->num_threads;
  Word * array = d->array;

  wlpdstm_thread_init();

  for (int j = 0; j < 25; j++) {
    BEGIN_TRANSACTION;

    for (int i = 0; i < ARRAY_SZ; ++i) {
      if ((i % nthreads) >= me) {
        Word cur = wlpdstm_read_word(&array[i]);
        wlpdstm_write_word(&array[i], cur);
      }
    }

    END_TRANSACTION;
  }
  wlpdstm_thread_shutdown();   
  return NULL;
}

void print_time(struct timeval tv1, struct timeval tv2) {
  printf("%f\n", (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000000.0);
}

int main() {
  wlpdstm_global_init();
  wlpdstm_thread_init();


  struct timeval tv1, tv2;
  struct rusage ru1, ru2;

  gettimeofday(&tv1, NULL);
  getrusage(RUSAGE_SELF, &ru1);

  pthread_t threads[4];
  arg_t arg[4];
  Word *array = (Word *)wlpdstm_tx_malloc(sizeof(Word) * ARRAY_SZ);
  for (int i = 0; i < 4; i++) {
    arg[i].thread_id = i;
    arg[i].num_threads = 4;
    arg[i].array = array;
    pthread_create(&threads[i], NULL, InterferingWrites, &arg[i]);
  }
  for (int i = 0; i < 4;i ++) {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&tv2, NULL);
  getrusage(RUSAGE_SELF, &ru2);

  printf("real time: ");
  print_time(tv1, tv2);
  printf("utime: ");
  print_time(ru1.ru_utime, ru2.ru_utime);
  printf("stime: ");
  print_time(ru1.ru_stime, ru2.ru_stime);

  wlpdstm_thread_shutdown();
  wlpdstm_global_shutdown();
  return 0;
}
