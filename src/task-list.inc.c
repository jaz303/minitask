void task_list_insert_before(struct minitask_list *lst, minitask_task_t *task, minitask_task_t *before) {
    if (lst->count == 0) {
        assert(before == NULL);
        task->next = task;
        task->prev = task;
        lst->head = task;
        lst->count = 1;
    } else {
        task->next = before;
        task->prev = before->prev;
        before->prev->next = task;
        before->prev = task;
        lst->count++;
    }
}

void task_list_append(struct minitask_list *lst, minitask_task_t *task) {
    task_list_insert_before(lst, task, lst->head);
}

void task_list_remove(struct minitask_list *lst, minitask_task_t *task) {
    if (lst->count == 1) {
        assert(task->next == task);
        assert(task->prev == task);
        lst->count = 0;
        lst->head = NULL;
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
        if (lst->head == task) {
            lst->head = task->next;
        }
        lst->count--;
    }
}