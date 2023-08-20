#pragma once

#if USE_PTHREADS

#include <pthread.h>

#define mutex_t pthread_mutex_t
#define mutex_init(m) pthread_mutex_init(m, NULL)
#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define mutex_trylock(m) (!pthread_mutex_trylock(m))
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock

#else

#include <pthread.h>
#include <stdbool.h>
#include "atomic.h"
#include "futex.h"
#include "spinlock.h"

typedef struct {
    atomic int state;
    pthread_t owner;
    pthread_t own_prio;

} mutex_t;

enum {
    MUTEX_LOCKED = 1 << 0,
    MUTEX_SLEEPING = 1 << 1,
};

#define MUTEX_INITIALIZER \
    {                     \
        .state = 0        \
        .own_prio = 0     \
    }

static inline void mutex_init(mutex_t *mutex)
{
    atomic_init(&mutex->state, 0);
    mutex->own_prio = 0;
}

static bool mutex_trylock(mutex_t *mutex)
{
    int state = load(&mutex->state, relaxed);
    if (state & MUTEX_LOCKED)
        return false;

    state = fetch_or(&mutex->state, MUTEX_LOCKED, relaxed);
    if (state & MUTEX_LOCKED)
        return false;

    thread_fence(&mutex->state, acquire);
    return true;
}

static inline void mutex_lock(mutex_t *mutex)
{
#define MUTEX_SPINS 128
    for (int i = 0; i < MUTEX_SPINS; ++i) {
        if (mutex_trylock(mutex))
            return;
        spin_hint();
    }
    int own_policy;
    struct sched_param own_param;

    int state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);

    while (state & MUTEX_LOCKED) {
        int policy;
        struct sched_param param;
        pthread_getschedparam(mutex->owner, &own_policy, &own_param);
        pthread_getschedparam(pthread_self(), &policy, &param);
        if (own_param.sched_priority < param.sched_priority) {
            pthread_setschedparam(mutex->owner, SCHED_RR, &param);
        }
        futex_wait(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING);
        state = exchange(&mutex->state, MUTEX_LOCKED | MUTEX_SLEEPING, relaxed);
    }

    thread_fence(&mutex->state, acquire);
    mutex->owner = pthread_self();
    pthread_getschedparam(mutex->owner, &own_policy, &own_param);
    mutex->own_prio = own_param.sched_priority;
}

static inline void mutex_unlock(mutex_t *mutex)
{
    int state = exchange(&mutex->state, 0, release);
    if (state & MUTEX_SLEEPING) {
        pthread_setschedprio(mutex->owner, mutex->own_prio);
        // FFFF(&mutex->state, 1);
        futex_wake(&mutex->state, 1);
    }
    
}

#endif