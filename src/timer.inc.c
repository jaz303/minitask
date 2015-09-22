pthread_t timer_thread;
pthread_mutex_t timeout_lock;
pthread_cond_t timeout_signal;

struct timeout;
struct timeout {
    minitask_task_t *task;
    struct timespec abstime;
    struct timeout *next;
};

struct timeout *timeouts = NULL;

struct timeout* timer_make_timeout(minitask_task_t *task) {
    struct timeout *to = malloc(sizeof(struct timeout));
    if (to == NULL) {
        fprintf(stderr, "failed to allocate memory for timeout\n");
        exit(1);
    }
    to->task = task;
    return to;
}

void timer_destroy_timeout(struct timeout *to) {
    free(to);
}

int timer_is_due(struct timeout *t) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec > t->abstime.tv_sec)
            || (tv.tv_sec == t->abstime.tv_sec
                && tv.tv_usec >= (t->abstime.tv_nsec / 1000));
}

void* timer_main(void *_) {
    while (1) {
        pthread_mutex_lock(&timeout_lock);
        while (!timeouts) {
            pthread_cond_wait(&timeout_signal, &timeout_lock);
        }
        while (!timer_is_due(timeouts)) {
            pthread_cond_timedwait(&timeout_signal, &timeout_lock, &timeouts->abstime);
        }
        struct timeout *triggered = timeouts;
        timeouts = timeouts->next;
        pthread_mutex_unlock(&timeout_lock);
        scheduler_wakeup_task(triggered->task);
        timer_destroy_timeout(triggered);
    }
    return NULL;
}

void timer_insert(struct timeout *to) {
    pthread_mutex_lock(&timeout_lock);
    
    struct timeout *curr = timeouts;
    struct timeout *prev = NULL;
    while (curr) {
        if (to->abstime.tv_sec < curr->abstime.tv_sec
            || (to->abstime.tv_sec == curr->abstime.tv_sec
                && to->abstime.tv_nsec < curr->abstime.tv_nsec)) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if (!prev) {
        to->next = timeouts;
        timeouts = to;
    } else {
        to->next = prev->next;
        prev->next = to;
    }

    pthread_mutex_unlock(&timeout_lock);
    pthread_cond_signal(&timeout_signal);
}

void timer_notify_s(minitask_task_t *task, int s) {
    struct timeout *to = timer_make_timeout(task);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    to->abstime.tv_sec = tv.tv_sec + s;
    to->abstime.tv_nsec = tv.tv_usec * 1000;
    timer_insert(to);
}

void timer_notify_us(minitask_task_t *task, int us) {
    struct timeout *to = timer_make_timeout(task);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    to->abstime.tv_sec = tv.tv_sec + (us / 1000000);
    to->abstime.tv_nsec = (tv.tv_usec * 1000) + ((us % 1000000) * 1000);
    if (to->abstime.tv_nsec >= 1000000000) {
        to->abstime.tv_sec += (to->abstime.tv_nsec / 1000000000);
        to->abstime.tv_nsec %= 1000000000;
    }
    timer_insert(to);
}

void timer_notify_ms(minitask_task_t *task, int ms) {
    timer_notify_us(task, ms * 1000);
}

int timer_start() {
    if (pthread_mutex_init(&timeout_lock, NULL) != 0
        || pthread_cond_init(&timeout_signal, NULL) != 0
        || pthread_create(&timer_thread, NULL, timer_main, NULL) != 0) {
        return -1;
    }
    return 0;
}
