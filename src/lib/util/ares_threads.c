/* MIT License
 *
 * Copyright (c) 2023 Brad House
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ares_private.h"
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#ifdef CARES_THREADS
#  ifdef _WIN32

struct ares_thread_mutex {
  CRITICAL_SECTION mutex;
};

ares_thread_mutex_t *ares_thread_mutex_create(void)
{
  ares_thread_mutex_t *mut = ares_malloc_zero(sizeof(*mut));
  if (mut == NULL) {
    return NULL;
  }

  InitializeCriticalSection(&mut->mutex);
  return mut;
}

void ares_thread_mutex_destroy(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  DeleteCriticalSection(&mut->mutex);
  ares_free(mut);
}

void ares_thread_mutex_lock(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  EnterCriticalSection(&mut->mutex);
}

void ares_thread_mutex_unlock(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  LeaveCriticalSection(&mut->mutex);
}

#    if _WIN32_WINNT >= 0x0600 /* Vista */

struct ares_thread_cond {
  CONDITION_VARIABLE cond;
};

ares_thread_cond_t *ares_thread_cond_create(void)
{
  ares_thread_cond_t *cond = ares_malloc_zero(sizeof(*cond));
  if (cond == NULL) {
    return NULL;
  }
  InitializeConditionVariable(&cond->cond);
  return cond;
}

void ares_thread_cond_destroy(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  ares_free(cond);
}

void ares_thread_cond_signal(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  WakeConditionVariable(&cond->cond);
}

void ares_thread_cond_broadcast(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  WakeAllConditionVariable(&cond->cond);
}

ares_status_t ares_thread_cond_wait(ares_thread_cond_t  *cond,
                                    ares_thread_mutex_t *mut)
{
  if (cond == NULL || mut == NULL) {
    return ARES_EFORMERR;
  }

  SleepConditionVariableCS(&cond->cond, &mut->mutex, INFINITE);
  return ARES_SUCCESS;
}

ares_status_t ares_thread_cond_timedwait(ares_thread_cond_t  *cond,
                                         ares_thread_mutex_t *mut,
                                         size_t               timeout_ms)
{
  DWORD tout;

  if (cond == NULL || mut == NULL) {
    return ARES_EFORMERR;
  }

  if (timeout_ms == SIZE_MAX) {
    tout = INFINITE;
  } else {
    tout = (DWORD)timeout_ms;
  }

  if (!SleepConditionVariableCS(&cond->cond, &mut->mutex, tout)) {
    return ARES_ETIMEOUT;
  }

  return ARES_SUCCESS;
}

#    else

typedef enum {
  ARES_W32_COND_SIGNAL    = 0,
  ARES_W32_COND_BROADCAST,
  ARES_W32_COND_EVMAX,
  ARES_W32_COND_NONE = ARES_W32_COND_EVMAX
} ares_w32_cond_event_t;

struct ares_thread_cond {
  HANDLE                events[2];
  HANDLE                gate;
  CRITICAL_SECTION      mutex;
  size_t                waiters;
  ares_w32_cond_event_t event;
};

ares_thread_cond_t *ares_thread_cond_create(void)
{
  ares_thread_cond_t *cond;

  cond = ares_malloc_zero(sizeof(*cond));
  if (cond == NULL) {
    return NULL;
  }

  cond->events[ARES_W32_COND_SIGNAL] = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (cond->events[ARES_W32_COND_SIGNAL] == NULL) {
    goto fail;
  }

  cond->events[ARES_W32_COND_BROADCAST] = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (cond->events[ARES_W32_COND_BROADCAST] == NULL) {
    goto fail;
  }

  /* Use a semaphore as a gate so we don't lose signals */
  cond->gate = CreateSemaphore(NULL, 1, 1, NULL);
  if (cond->gate == NULL) {
    goto fail;
  }

  InitializeCriticalSection(&cond->mutex);
  cond->waiters = 0;
  cond->event   = ARES_W32_COND_NONE;

  return cond;

fail:
  ares_thread_cond_destroy(cond);
  return NULL;
}

void ares_thread_cond_destroy(ares_thread_cond_t *cond)
{
  if (cond == NULL)
    return;

  if (cond->events[ARES_W32_COND_SIGNAL]) {
    CloseHandle(cond->events[ARES_W32_COND_SIGNAL]);
  }
  if (cond->events[ARES_W32_COND_BROADCAST]) {
    CloseHandle(cond->events[ARES_W32_COND_BROADCAST]);
  }
  if (cond->gate) {
    CloseHandle(cond->gate);
  }
  DeleteCriticalSection(&cond->mutex);

  ares_free(cond);
}

ares_status_t ares_thread_cond_timedwait(ares_thread_cond_t  *cond,
                                         ares_thread_mutex_t *mut,
                                         size_t               timeout_ms)
{
  DWORD rv;
  DWORD dwMilliseconds;

  if (cond == NULL || mut == NULL) {
    return ARES_EFORMERR;
  }

  /* We may only enter when no wakeups active this will prevent the lost
   * wakeup */
  WaitForSingleObject(cond->gate, INFINITE);

  EnterCriticalSection(&cond->mutex);
  /* count waiters passing through */
  cond->waiters++;
  LeaveCriticalSection(&cond->mutex);

  /* Open Gate */
  ReleaseSemaphore(cond->gate, 1, NULL);

  /* Release passed in mutex */
  ares_thread_mutex_unlock(mut);

  if (timeout_ms == SIZE_MAX) {
    dwMilliseconds = INFINITE;
  } else {
    dwMilliseconds = (DWORD)timeout_ms;
  }
  rv = WaitForMultipleObjects(ARES_W32_COND_EVMAX, cond->events, FALSE,
                              dwMilliseconds);

  /* We go into a critical section to make sure cond->waiters isn't checked
   * while we decrement.  This is especially important for a timeout since the
   * gate may not be closed. We need to check to see if a broadcast/signal was
   * pending as this thread could have been preempted prior to
   * EnterCriticalSection but after WaitForMultipleObjects() so we may be
   * responsible for resetting the event and closing the gate */
  EnterCriticalSection(&cond->mutex);
  cond->waiters--;

  if (cond->event != ARES_W32_COND_NONE && cond->waiters == 0) {
    /* Last waiter needs to reset the event on(as a broadcast event is not
     * automatic) and also re-open the gate */
    if (cond->event == ARES_W32_COND_BROADCAST) {
      ResetEvent(cond->events[ARES_W32_COND_BROADCAST]);
    }

    /* Open Gate (closed by ares_thread_cond_broadcast()) since there are no
     * more waiters for the event */
    ReleaseSemaphore(cond->gate, 1, NULL);
    cond->event = ARES_W32_COND_NONE;
  } else if (rv == WAIT_OBJECT_0 + ARES_W32_COND_SIGNAL) {
    /* If specifically, this thread was signalled and there are more waiting,
     * re-open the gate and reset the event */
    ReleaseSemaphore(cond->gate, 1, NULL);
    cond->event = ARES_W32_COND_NONE;
  } else {
    /* This could be a standard timeout with more waiters, don't do anything */
  }
  LeaveCriticalSection(&cond->mutex);

  /* re-lock the passed in mutex */
  ares_thread_mutex_lock(mut);

  if (rv == WAIT_TIMEOUT) {
    return ARES_ETIMEOUT;
  }

  return ARES_SUCCESS;
}

ares_status_t ares_thread_cond_wait(ares_thread_cond_t  *cond,
                                    ares_thread_mutex_t *mut)
{
  return ares_thread_cond_timedwait(cond, mut, SIZE_MAX);
}

void ares_thread_cond_broadcast(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }

  /* close gate to prevent more waiters while broadcasting */
  WaitForSingleObject(cond->gate, INFINITE);

  /* If there are waiters, send a broadcast event,
   * otherwise, just reopen the gate */
  EnterCriticalSection(&cond->mutex);

  cond->event = ARES_W32_COND_BROADCAST;
  if (cond->waiters) {
    /* wake all waiters */
    SetEvent(cond->events[ARES_W32_COND_BROADCAST]);
  } else {
    /* if no waiters just reopen gate */
    ReleaseSemaphore(cond->gate, 1, NULL);
  }

  LeaveCriticalSection(&cond->mutex);
}

void ares_thread_cond_signal(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }

  /* close gate to prevent more waiters while signalling */
  WaitForSingleObject(cond->gate, INFINITE);

  EnterCriticalSection(&cond->mutex);
  cond->event = ARES_W32_COND_SIGNAL;
  if (cond->waiters) {
    /* wake one waiter */
    SetEvent(cond->events[ARES_W32_COND_SIGNAL]);
  } else {
    /* no waiters, just reopen the gate */
    ReleaseSemaphore(cond->gate, 1, NULL);
  }
  LeaveCriticalSection(&cond->mutex);
}

#    endif

struct ares_thread {
  HANDLE thread;
  DWORD  id;

  void *(*func)(void *arg);
  void *arg;
  void *rv;
};

/* Wrap for pthread compatibility */
static DWORD WINAPI ares_thread_func(LPVOID lpParameter)
{
  ares_thread_t *thread = lpParameter;

  thread->rv = thread->func(thread->arg);
  return 0;
}

ares_status_t ares_thread_create(ares_thread_t    **thread,
                                 ares_thread_func_t func, void *arg)
{
  ares_thread_t *thr = NULL;

  if (func == NULL || thread == NULL) {
    return ARES_EFORMERR;
  }

  thr = ares_malloc_zero(sizeof(*thr));
  if (thr == NULL) {
    return ARES_ENOMEM;
  }

  thr->func   = func;
  thr->arg    = arg;
  thr->thread = CreateThread(NULL, 0, ares_thread_func, thr, 0, &thr->id);
  if (thr->thread == NULL) {
    ares_free(thr);
    return ARES_ESERVFAIL;
  }

  *thread = thr;
  return ARES_SUCCESS;
}

ares_status_t ares_thread_join(ares_thread_t *thread, void **rv)
{
  ares_status_t status = ARES_SUCCESS;

  if (thread == NULL) {
    return ARES_EFORMERR;
  }

  if (WaitForSingleObject(thread->thread, INFINITE) != WAIT_OBJECT_0) {
    status = ARES_ENOTFOUND;
  } else {
    CloseHandle(thread->thread);
  }

  if (status == ARES_SUCCESS && rv != NULL) {
    *rv = thread->rv;
  }
  ares_free(thread);

  return status;
}

#  else /* !WIN32 == PTHREAD */
#    include <pthread.h>

/* for clock_gettime() */
#    ifdef HAVE_TIME_H
#      include <time.h>
#    endif

/* for gettimeofday() */
#    ifdef HAVE_SYS_TIME_H
#      include <sys/time.h>
#    endif

struct ares_thread_mutex {
  pthread_mutex_t mutex;
};

ares_thread_mutex_t *ares_thread_mutex_create(void)
{
  pthread_mutexattr_t  attr;
  ares_thread_mutex_t *mut = ares_malloc_zero(sizeof(*mut));
  if (mut == NULL) {
    return NULL;
  }

  if (pthread_mutexattr_init(&attr) != 0) {
    ares_free(mut); /* LCOV_EXCL_LINE: UntestablePath */
    return NULL;    /* LCOV_EXCL_LINE: UntestablePath */
  }

  if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
    goto fail; /* LCOV_EXCL_LINE: UntestablePath */
  }

  if (pthread_mutex_init(&mut->mutex, &attr) != 0) {
    goto fail; /* LCOV_EXCL_LINE: UntestablePath */
  }

  pthread_mutexattr_destroy(&attr);
  return mut;

/* LCOV_EXCL_START: UntestablePath */
fail:
  pthread_mutexattr_destroy(&attr);
  ares_free(mut);
  return NULL;
  /* LCOV_EXCL_STOP */
}

void ares_thread_mutex_destroy(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  pthread_mutex_destroy(&mut->mutex);
  ares_free(mut);
}

void ares_thread_mutex_lock(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  pthread_mutex_lock(&mut->mutex);
}

void ares_thread_mutex_unlock(ares_thread_mutex_t *mut)
{
  if (mut == NULL) {
    return;
  }
  pthread_mutex_unlock(&mut->mutex);
}

struct ares_thread_cond {
  pthread_cond_t cond;
};

ares_thread_cond_t *ares_thread_cond_create(void)
{
  ares_thread_cond_t *cond = ares_malloc_zero(sizeof(*cond));
  if (cond == NULL) {
    return NULL;
  }
  pthread_cond_init(&cond->cond, NULL);
  return cond;
}

void ares_thread_cond_destroy(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  pthread_cond_destroy(&cond->cond);
  ares_free(cond);
}

void ares_thread_cond_signal(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  pthread_cond_signal(&cond->cond);
}

void ares_thread_cond_broadcast(ares_thread_cond_t *cond)
{
  if (cond == NULL) {
    return;
  }
  pthread_cond_broadcast(&cond->cond);
}

ares_status_t ares_thread_cond_wait(ares_thread_cond_t  *cond,
                                    ares_thread_mutex_t *mut)
{
  if (cond == NULL || mut == NULL) {
    return ARES_EFORMERR;
  }

  pthread_cond_wait(&cond->cond, &mut->mutex);
  return ARES_SUCCESS;
}

static void ares_timespec_timeout(struct timespec *ts, size_t add_ms)
{
#    if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
  clock_gettime(CLOCK_REALTIME, ts);
#    elif defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ts->tv_sec  = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000;
#    else
#      error cannot determine current system time
#    endif

  ts->tv_sec  += (time_t)(add_ms / 1000);
  ts->tv_nsec += (long)((add_ms % 1000) * 1000000);

  /* Normalize if needed */
  if (ts->tv_nsec >= 1000000000) {
    ts->tv_sec  += ts->tv_nsec / 1000000000;
    ts->tv_nsec %= 1000000000;
  }
}

ares_status_t ares_thread_cond_timedwait(ares_thread_cond_t  *cond,
                                         ares_thread_mutex_t *mut,
                                         size_t               timeout_ms)
{
  struct timespec ts;

  if (cond == NULL || mut == NULL) {
    return ARES_EFORMERR;
  }

  if (timeout_ms == SIZE_MAX) {
    return ares_thread_cond_wait(cond, mut);
  }

  ares_timespec_timeout(&ts, timeout_ms);

  if (pthread_cond_timedwait(&cond->cond, &mut->mutex, &ts) != 0) {
    return ARES_ETIMEOUT;
  }

  return ARES_SUCCESS;
}

struct ares_thread {
  pthread_t thread;
};

ares_status_t ares_thread_create(ares_thread_t    **thread,
                                 ares_thread_func_t func, void *arg)
{
  ares_thread_t *thr = NULL;

  if (func == NULL || thread == NULL) {
    return ARES_EFORMERR;
  }

  thr = ares_malloc_zero(sizeof(*thr));
  if (thr == NULL) {
    return ARES_ENOMEM; /* LCOV_EXCL_LINE: OutOfMemory */
  }
  if (pthread_create(&thr->thread, NULL, func, arg) != 0) {
    ares_free(thr);        /* LCOV_EXCL_LINE: UntestablePath */
    return ARES_ESERVFAIL; /* LCOV_EXCL_LINE: UntestablePath */
  }

  *thread = thr;
  return ARES_SUCCESS;
}

ares_status_t ares_thread_join(ares_thread_t *thread, void **rv)
{
  void         *ret    = NULL;
  ares_status_t status = ARES_SUCCESS;

  if (thread == NULL) {
    return ARES_EFORMERR;
  }

  if (pthread_join(thread->thread, &ret) != 0) {
    status = ARES_ENOTFOUND;
  }
  ares_free(thread);

  if (status == ARES_SUCCESS && rv != NULL) {
    *rv = ret;
  }
  return status;
}

#  endif

ares_bool_t ares_threadsafety(void)
{
  return ARES_TRUE;
}

#else /* !CARES_THREADS */

/* NoOp */
ares_thread_mutex_t *ares_thread_mutex_create(void)
{
  return NULL;
}

void ares_thread_mutex_destroy(ares_thread_mutex_t *mut)
{
  (void)mut;
}

void ares_thread_mutex_lock(ares_thread_mutex_t *mut)
{
  (void)mut;
}

void ares_thread_mutex_unlock(ares_thread_mutex_t *mut)
{
  (void)mut;
}

ares_thread_cond_t *ares_thread_cond_create(void)
{
  return NULL;
}

void ares_thread_cond_destroy(ares_thread_cond_t *cond)
{
  (void)cond;
}

void ares_thread_cond_signal(ares_thread_cond_t *cond)
{
  (void)cond;
}

void ares_thread_cond_broadcast(ares_thread_cond_t *cond)
{
  (void)cond;
}

ares_status_t ares_thread_cond_wait(ares_thread_cond_t  *cond,
                                    ares_thread_mutex_t *mut)
{
  (void)cond;
  (void)mut;
  return ARES_ENOTIMP;
}

ares_status_t ares_thread_cond_timedwait(ares_thread_cond_t  *cond,
                                         ares_thread_mutex_t *mut,
                                         size_t               timeout_ms)
{
  (void)cond;
  (void)mut;
  (void)timeout_ms;
  return ARES_ENOTIMP;
}

ares_status_t ares_thread_create(ares_thread_t    **thread,
                                 ares_thread_func_t func, void *arg)
{
  (void)thread;
  (void)func;
  (void)arg;
  return ARES_ENOTIMP;
}

ares_status_t ares_thread_join(ares_thread_t *thread, void **rv)
{
  (void)thread;
  (void)rv;
  return ARES_ENOTIMP;
}

ares_bool_t ares_threadsafety(void)
{
  return ARES_FALSE;
}
#endif


ares_status_t ares_channel_threading_init(ares_channel_t *channel)
{
  ares_status_t status = ARES_SUCCESS;

  /* Threading is optional! */
  if (!ares_threadsafety()) {
    return ARES_SUCCESS;
  }

  channel->lock = ares_thread_mutex_create();
  if (channel->lock == NULL) {
    status = ARES_ENOMEM;
    goto done;
  }

  channel->cond_empty = ares_thread_cond_create();
  if (channel->cond_empty == NULL) {
    status = ARES_ENOMEM;
    goto done;
  }

done:
  if (status != ARES_SUCCESS) {
    ares_channel_threading_destroy(channel);
  }
  return status;
}

void ares_channel_threading_destroy(ares_channel_t *channel)
{
  ares_thread_mutex_destroy(channel->lock);
  channel->lock = NULL;
  ares_thread_cond_destroy(channel->cond_empty);
  channel->cond_empty = NULL;
}

void ares_channel_lock(const ares_channel_t *channel)
{
  ares_thread_mutex_lock(channel->lock);
}

void ares_channel_unlock(const ares_channel_t *channel)
{
  ares_thread_mutex_unlock(channel->lock);
}

/* Must not be holding a channel lock already, public function only */
ares_status_t ares_queue_wait_empty(ares_channel_t *channel, int timeout_ms)
{
  ares_status_t  status = ARES_SUCCESS;
  ares_timeval_t tout;

  if (!ares_threadsafety()) {
    return ARES_ENOTIMP;
  }

  if (channel == NULL) {
    return ARES_EFORMERR;
  }

  if (timeout_ms >= 0) {
    ares_tvnow(&tout);
    tout.sec  += (ares_int64_t)(timeout_ms / 1000);
    tout.usec += (unsigned int)(timeout_ms % 1000) * 1000;
  }

  ares_thread_mutex_lock(channel->lock);
  while (ares_llist_len(channel->all_queries)) {
    if (timeout_ms < 0) {
      ares_thread_cond_wait(channel->cond_empty, channel->lock);
    } else {
      ares_timeval_t tv_remaining;
      ares_timeval_t tv_now;
      unsigned long  tms;

      ares_tvnow(&tv_now);
      ares_timeval_remaining(&tv_remaining, &tv_now, &tout);
      tms =
        (unsigned long)((tv_remaining.sec * 1000) + (tv_remaining.usec / 1000));
      if (tms == 0) {
        status = ARES_ETIMEOUT;
      } else {
        status =
          ares_thread_cond_timedwait(channel->cond_empty, channel->lock, tms);
      }

      /* If there was a timeout, don't loop.  Otherwise, make sure this wasn't
       * a spurious wakeup by looping and checking the condition. */
      if (status == ARES_ETIMEOUT) {
        break;
      }
    }
  }
  ares_thread_mutex_unlock(channel->lock);
  return status;
}

void ares_queue_notify_empty(ares_channel_t *channel)
{
  if (channel == NULL) {
    return;
  }

  /* We are guaranteed to be holding a channel lock already */
  if (ares_llist_len(channel->all_queries)) {
    return;
  }

  /* Notify all waiters of the conditional */
  ares_thread_cond_broadcast(channel->cond_empty);
}
