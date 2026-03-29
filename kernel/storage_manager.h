#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

void sys_save_task(int task_id, int slot);
int sys_load_task(int task_id, int slot);
int storage_snapshot_used_count(void);
int storage_snapshot_capacity(void);
int storage_set_snapshot_tag(int slot, const char *tag);
int storage_get_snapshot_tag(int slot, char *out, int out_len);
int storage_get_snapshot_signature(int slot, int *voltage_sum, int *weight_sum,
								   int *threshold_sum);
int storage_diff_snapshots(int slot_a, int slot_b, int *voltage_delta,
						   int *weight_delta, int *threshold_delta);

#endif
