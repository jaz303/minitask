#ifndef MINITASK_H
#define MINITASK_H

typedef void (minitask_entry_f)(void*);

void minitask_init();
int minitask_active_task_id();
void minitask_sleep_ms(int delay_ms);
void minitask_spawn(minitask_entry_f fn, void *arg);
void minitask_yield();
void minitask_start();

#endif