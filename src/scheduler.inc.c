#define STACK_SIZE SIGSTKSZ

int scheduler_init(minitask_scheduler_t *s) {
	memset(s, 0, sizeof(minitask_scheduler_t));
	if (pthread_mutex_init(&s->wake.lock, NULL) != 0
		|| pthread_cond_init(&s->wake.signal, NULL) != 0) {
		return -1;
	}
	return 0;
}

minitask_task_t* scheduler_active_task(minitask_scheduler_t *s) {
	return s->active_task;
}

void scheduler_wakeup_task(minitask_task_t *task) {
	printf("waking up task ID: %d\n", task->id);
	minitask_scheduler_t *s = task->scheduler;
	pthread_mutex_lock(&s->wake.lock);
	task->next_woken = s->wake.head;
	s->wake.head = task;
	s->wake.pending = 1;
	pthread_mutex_unlock(&s->wake.lock);
	pthread_cond_signal(&s->wake.signal);
}

void scheduler_enqueue_woken_tasks(minitask_scheduler_t *s) {
	while (s->wake.head) {
		task_list_remove(&s->waiting, s->wake.head);
		task_list_append(&s->active, s->wake.head);
		s->wake.head = s->wake.head->next_woken;
	}
	s->wake.pending = 0;
}

void scheduler_run_task(minitask_scheduler_t *s, minitask_task_t *task) {
	minitask_task_t *previous_task = s->active_task;
	if (task == previous_task) {
		return;
	}
	s->active_task = task;
	swapcontext(&previous_task->ctx, &task->ctx);
}

void scheduler_switch(minitask_scheduler_t *s) {
	while (1) {
		if (s->wake.pending) {
			pthread_mutex_lock(&s->wake.lock);
			scheduler_enqueue_woken_tasks(s);
			pthread_mutex_unlock(&s->wake.lock);
		}
		if (s->active.count) {
			scheduler_run_task(s, s->active.head);
			break;
		} else if (s->waiting.count) {
			pthread_mutex_lock(&s->wake.lock);
			while (!s->wake.pending) {
				pthread_cond_wait(&s->wake.signal, &s->wake.lock);
			}
			scheduler_enqueue_woken_tasks(s);
			pthread_mutex_unlock(&s->wake.lock);
		} else {
			// no waiting tasks and no active tasks;
			// this thread can be returned to the pool, when such
			// a thing exists...
			pthread_mutex_lock(&s->wake.lock);
			printf("entering idle state...\n");
			while (1) {
				pthread_cond_wait(&s->wake.signal, &s->wake.lock);
			}
		}
	}
}

#if defined(PACK_EQUAL)
void scheduler_start_task(int task_int) {
	minitask_task_t *task = (minitask_task_t*)task_int;

#elif defined(PACK_64_32)
void scheduler_start_task(uint32_t task_high, uint32_t task_low) {
	uint64_t task_int = ((uint64_t)task_high << 32) | task_low;
	minitask_task_t *task = (minitask_task_t*)task_int;

#else
void scheduler_start_task() {
	minitask_task_t *task = NULL;

#error "no packing configured"
#endif
	
	minitask_scheduler_t *s = task->scheduler;
	task->entry(task->arg);
	assert(task == s->active_task);
	task_list_remove(&s->active, task);
	free(task->stack);
	free(task);
	scheduler_switch(s);
}

void scheduler_spawn(minitask_scheduler_t *s, minitask_entry_f fn, void *arg) {
	minitask_task_t *task = malloc(sizeof(minitask_task_t));
	if (!task) {
		fprintf(stderr, "minitask: failed to allocated task\n");
		exit(1);
	}

	task->stack = malloc(STACK_SIZE);
	if (!task->stack) {
		fprintf(stderr, "minitask: failed to allocate task stack\n");
		exit(1);
	}

	task->id = ++(s->next_task_id);
	task->entry = fn;
	task->arg = arg;
	task->scheduler = s;
	task->prev = NULL;
	task->next = NULL;
	task->next_woken = NULL;

	if (getcontext(&task->ctx) != 0) {
		fprintf(stderr, "minitask: getcontext() failed\n");
		exit(1);
	}

	task->ctx.uc_link = 0;
	task->ctx.uc_stack.ss_sp = task->stack;
	task->ctx.uc_stack.ss_size = STACK_SIZE;
	task->ctx.uc_stack.ss_flags = 0;

#if defined(PACK_EQUAL)
	makecontext(&task->ctx, scheduler_start_task, 1, (int)task);
#elif defined(PACK_64_32)
	makecontext(&task->ctx, scheduler_start_task, 2, (uint32_t)(((uint64_t)task) >> 32), (uint32_t)task);
#else
#error "no packing configured"
#endif
	
	task_list_append(&s->active, task);
}

void scheduler_yield(minitask_scheduler_t *s) {
	s->active.head = s->active.head->next;
	scheduler_switch(s);
}

void scheduler_wait(minitask_scheduler_t *s) {
	task_list_remove(&s->active, s->active_task);
	task_list_append(&s->waiting, s->active_task);
}

void scheduler_sleep_ms(minitask_scheduler_t *s, int ms) {
	scheduler_wait(s);
	timer_notify_ms(s->active_task, ms);
	scheduler_switch(s);
}

void scheduler_start(minitask_scheduler_t *s) {
	s->active_task = s->active.head;
	setcontext(&s->active_task->ctx);
}
