#include "thread.h"
#include "errcode.h"


err_t DC_thread_create(__out__ DC_thread_t *t, __in__ DC_thread_proc_t proc, __in__ void *param) {
#ifdef OS_WINDOWS
    if (ISNULL((*t = CreateThread(NULL, 0, proc, param, 0, NULL)))) {
        return E_ERROR;
    }
#else
    if (ISERR(pthread_create(t, NULL, proc, param))) {
        return E_ERROR;
    }
#endif

    return E_OK;
}

void DC_thread_cancel(__in__ DC_thread_t t) {
#ifndef OS_WINDOWS
    pthread_cancel(t);
#endif
}


void DC_thread_exit(__in__ uint32_t code) {
#ifdef OS_WINDOWS
    ExitThread(code);
#else
    pthread_exit(&code);
#endif
}

err_t DC_thread_join(__in__ DC_thread_t t, __out__ uint32_t *exitcode) {
#ifdef OS_WINDOWS
    *exitcode = WaitForSingleObject(t, INFINITE);
    if (*exitcode == WAIT_FAILED) return E_ERROR;
#else
    int *code = 0;
    if (ISERR(pthread_join(t, (void **) &code))) {
        return E_ERROR;
    }

    *exitcode = *code;
#endif

    return E_OK;
}

err_t DC_thread_mutex_init(__in__ DC_thread_mutex_t *t) {
#ifdef OS_WINDOWS
    InitializeCriticalSection(t);
#else
    if (ISERR(pthread_mutex_init(t, NULL))) {
        return E_ERROR;
    }
#endif

    return E_OK;
}

err_t DC_thread_mutex_lock(__in__ DC_thread_mutex_t *t) {
#ifdef OS_WINDOWS
    EnterCriticalSection(t);
#else
    if (ISERR(pthread_mutex_lock(t))) {
        return E_ERROR;
    }
#endif
    return E_OK;
}

err_t DC_thread_mutex_trylock(__in__ DC_thread_mutex_t *t) {
#ifdef OS_WINDOWS
    if (!TryEnterCriticalSection(t)) {
        return E_ERROR;
    }
#else
    if (ISERR(pthread_mutex_trylock(t))) {
        return E_ERROR;
    }
#endif
    return E_OK;
}

void DC_thread_mutex_unlock(__in__ DC_thread_mutex_t *t) {
#ifdef OS_WINDOWS
    LeaveCriticalSection(t);
#else
    pthread_mutex_unlock(t);
#endif
}

void DC_thread_mutex_destroy(__in__ DC_thread_mutex_t *t) {
#ifdef OS_WINDOWS
    DeleteCriticalSection(t);
#else
    pthread_mutex_destroy(t);
#endif
}

err_t DC_thread_rwlock_init(__out__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    InitializeSRWLock(t);
#else
    if (ISERR(pthread_rwlock_init(t, NULL))) {
        return E_ERROR;
    }
#endif
    return E_OK;
}

err_t DC_thread_rwlock_rdlock(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    AcquireSRWLockExclusive(t);
#else
    if (ISERR(pthread_rwlock_rdlock(t))) {
        return E_ERROR;
    }
#endif
    return E_OK;
}

err_t DC_thread_rwlock_wrlock(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    AcquireSRWLockExclusive(t);
#else
    if (ISERR(pthread_rwlock_wrlock(t))) {
        return E_ERROR;
    }
#endif
    return E_OK;
}

err_t DC_thread_rwlock_tryrdlock(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    DC_set_errcode(E_UNSUPPORTED);
    return E_ERROR;
#else
    if (ISERR(pthread_rwlock_tryrdlock(t))) {
        return E_ERROR;
    }
#endif

    return E_OK;
}

err_t DC_thread_rwlock_trywrlock(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    DC_set_errcode(E_UNSUPPORTED);
    return E_ERROR;
#else
    if (ISERR(pthread_rwlock_trywrlock(t))) {
        return E_ERROR;
    }
#endif

    return E_OK;
}

void DC_thread_rwlock_unlock(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
    ReleaseSRWLockExclusive(t);
#else
    pthread_rwlock_unlock(t);
#endif
}

void DC_thread_rwlock_destroy(__in__ DC_thread_rwlock_t *t) {
#ifdef OS_WINDOWS
#else
    pthread_rwlock_destroy(t);
#endif
}

err_t DC_thread_cond_init(__out__ DC_thread_cond_t *t) {
#ifdef OS_WINDOWS
    InitializeConditionVariable(t);
#else
    return ISERR(pthread_cond_init(t, NULL)) ? E_ERROR : E_OK;
#endif
    return E_OK;
}

void DC_thread_cond_signal(__in__ DC_thread_cond_t *t, __in__ bool_t all) {
#ifdef OS_WINDOWS
    if (all) {
        WakeAllConditionVariable(t);
    }
    else {
        WakeConditionVariable(t);
    }
#else
    if (all) {
        pthread_cond_broadcast(t);
    } else {
        pthread_cond_signal(t);
    }
#endif
}

void DC_thread_cond_destroy(__in__ DC_thread_cond_t *t) {
#ifdef OS_WINDOWS
#else
    pthread_cond_destroy(t);
#endif
}

err_t DC_thread_cond_wait(DC_thread_cond_t *t,
                          DC_thread_mutex_t *m,
                          uint32_t ms) {
#ifdef OS_WINDOWS
    if (!SleepConditionVariableCS(t, m, ms > 0 ? ms : INFINITE)) {
        DC_set_errcode(E_TIMEDOUT);
        return E_ERROR;
    }
    return E_OK;
#else
    struct timespec __time_to_wait;
    struct timeval __now;


    if (ms > 0) {
        gettimeofday(&__now, NULL);
        __time_to_wait.tv_sec = __now.tv_sec + ((int) (ms / 1000));
        __time_to_wait.tv_nsec = __now.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
        __time_to_wait.tv_sec += __time_to_wait.tv_nsec / (1000 * 1000 * 1000);
        __time_to_wait.tv_nsec %= (1000 * 1000 * 1000);
        if (ISERR(pthread_cond_timedwait(t, m, &__time_to_wait))) {
            if (errno == ETIMEDOUT) {
                DC_set_errcode(E_TIMEDOUT);
            }
        }
    } else {
        if (ISERR(pthread_cond_wait(t, m))) {
            return E_ERROR;
        }

    }

    return E_OK;
#endif
}

