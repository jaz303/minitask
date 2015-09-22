minitask_scheduler_t scheduler;

int minitask_init() {
    timer_start();
    if (scheduler_init(&scheduler) != 0) {
        return -1;
    }
    return 0;
}

int minitask_active_task_id() {
    return scheduler_active_task(&scheduler)->id;
}

void minitask_sleep_ms(int ms) {
    scheduler_sleep_ms(&scheduler, ms);
}

void minitask_spawn(minitask_entry_f fn, void *arg) {
    scheduler_spawn(&scheduler, fn, arg);
}

void minitask_yield() {
    scheduler_yield(&scheduler);
}

void minitask_start() {
    scheduler_start(&scheduler);
}