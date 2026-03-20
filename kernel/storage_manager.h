#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

void sys_save_task(int task_id, int slot);
int sys_load_task(int task_id, int slot);
int storage_snapshot_used_count(void);
int storage_snapshot_capacity(void);

#endif
