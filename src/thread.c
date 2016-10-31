#include "thread.h"
#include "errcode.h"

#ifdef OS_WINDOWS
err_t _WIN_create_thread(PHANDLE *handle, DC_thread_proc_t proc, LPVOID param)
{
	*handle = CreateThread(NULL, 0, proc, param, 0, NULL);
	if (ISNULL(*handle)) {
		return E_ERROR;
	}


	return E_OK;
}
#else
#endif


err_t DC_thread_cond_timedwait(DC_thread_cond_t t,
                               DC_thread_mutex_t m,
                               uint32_t ms) {
    struct timespec __time_to_wait;
    struct timeval __now;

    gettimeofday(&__now, NULL);
    __time_to_wait.tv_sec = __now.tv_sec + ((int) (ms / 1000));
    __time_to_wait.tv_nsec = __now.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
    __time_to_wait.tv_sec += __time_to_wait.tv_nsec / (1000 * 1000 * 1000);
    __time_to_wait.tv_nsec %= (1000 * 1000 * 1000);

    if (ISERR (pthread_cond_timedwait(&t, &m, &__time_to_wait))) {
        if (errno == ETIMEDOUT) {
            return E_TIMEDOUT;
        }

        return E_SYSTEM;
    }

    return E_OK;
}

