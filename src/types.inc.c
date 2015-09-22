// sizeof(void*) == sizeof(int)
// #define PACK_EQUAL

// sizeof(void*) == 64, sizeof(int) == 32
#define PACK_64_32

typedef struct minitask_scheduler_t minitask_scheduler_t;
typedef struct minitask_task_t minitask_task_t;

typedef void (minitask_entry_f)(void*);

struct minitask_list {
    int count;
    minitask_task_t *head;
};

struct minitask_scheduler_t {
    int next_task_id;
    minitask_task_t *active_task;
    ucontext_t return_ctx;
    struct minitask_list active;
    struct minitask_list waiting;
    struct {
        int pending;
        pthread_mutex_t lock;
        pthread_cond_t signal;
        minitask_task_t *head;
    } wake;
};

struct minitask_task_t {
    int                     id;
    void                    *stack;
    minitask_entry_f        *entry;
    void                    *arg;
    ucontext_t              ctx;
    minitask_scheduler_t    *scheduler;
    minitask_task_t         *prev;
    minitask_task_t         *next;
    minitask_task_t         *next_woken;
};

void timer_notify_ms(minitask_task_t*, int);
void timer_notify_us(minitask_task_t*, int);
