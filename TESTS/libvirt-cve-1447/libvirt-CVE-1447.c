#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t init_lock, start_lock, ka_lock;

typedef int* virKeepAlivePtr;

struct virNetServerClient {
  int refs;
  virKeepAlivePtr keepalive;
};

typedef struct virNetServerClient* virNetServerClientPtr;

int *virKeepAliveNew()
{
  int *ret;
  ret = (int*)malloc(sizeof(int));
  *ret = 1;
  return ret;
}

int virNetServerClientInitKeepAlive(virNetServerClientPtr client)
{
  virKeepAlivePtr ka;
  int ret = -1;

  pthread_mutex_lock(&init_lock);

  ka = virKeepAliveNew();
  client->keepalive = ka;

  pthread_mutex_unlock(&init_lock);

  return ret;
}

int virKeepAliveStart(virKeepAlivePtr ka)
{
  int ret = 0;
  pthread_mutex_lock(&ka_lock);
  printf("use keepalive: %d\n", *ka);
  pthread_mutex_unlock(&ka_lock);
  return ret;
}

int virServerClientStartKeepAlive(virNetServerClientPtr client)
{
  int ret;

  pthread_mutex_lock(&start_lock);
  ret = virKeepAliveStart(client->keepalive);
  pthread_mutex_unlock(&start_lock);

  return ret;
}

void virNetServerClinetClose(virNetServerClientPtr client)
{
  virKeepAlivePtr ka;
  pthread_mutex_lock(&start_lock);
  if (client->keepalive) {
    ka = client->keepalive;
    client->keepalive = NULL;
    free(ka);
    pthread_mutex_unlock(&start_lock);
    return;
  }
  pthread_mutex_unlock(&start_lock);
}

int main()
{
  pthread_t thread1, thread2;

  pthread_mutex_init(&init_lock, NULL);
  pthread_mutex_init(&start_lock, NULL);
  pthread_mutex_init(&ka_lock, NULL);

  virNetServerClientPtr client = (virNetServerClientPtr)malloc(sizeof(struct virNetServerClient));

  virNetServerClientInitKeepAlive(client);

  pthread_create (&thread1, NULL, (void *) &virServerClientStartKeepAlive, (void *) client);
  pthread_create (&thread2, NULL, (void *) &virNetServerClinetClose, (void *) client);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  pthread_mutex_destroy(&init_lock);
  pthread_mutex_destroy(&start_lock);
  pthread_mutex_destroy(&ka_lock);

  return 0;
}

