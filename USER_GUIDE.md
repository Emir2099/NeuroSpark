# NeuroSpark OS — Complete User Guide

> **77 Root Commands · 150+ Sub-Commands · One Kernel**

This document is the authoritative reference for every operation available in the **Axiom Console**, NeuroSpark's built-in shell. Commands are organized by subsystem.

---

## Table of Contents

1. [System & Utility](#1-system--utility)
2. [Process & Job Control](#2-process--job-control-posix)
3. [File System & Storage](#3-file-system--storage)
4. [Page Cache](#4-page-cache)
5. [PCI Bus](#5-pci-bus)
6. [AHCI / SATA Disk Controller](#6-ahci--sata-disk-controller)
7. [USB (xHCI)](#7-usb-xhci)
8. [Audio (AC97) & Sonification](#8-audio-ac97--sonification)
9. [Networking (E1000 NIC)](#9-networking-e1000-nic)
10. [Remote Access & Authentication](#10-remote-access--authentication)
11. [Spiking Neural Network (Neuromorphic Engine)](#11-spiking-neural-network-neuromorphic-engine)
12. [AI Runtime & Inference](#12-ai-runtime--inference)
13. [Profiling & Diagnostics](#13-profiling--diagnostics)
14. [Kernel Modules](#14-kernel-modules)
15. [POSIX Identity & Permissions](#15-posix-identity--permissions)
16. [Scheduler](#16-scheduler)
17. [Memory](#17-memory)
18. [Visualization (NeuroViz)](#18-visualization-neuroviz)
19. [Synapse Inspector](#19-synapse-inspector)
20. [Snapshot Browser](#20-snapshot-browser)
21. [Data Management](#21-data-management)
22. [Reproducibility](#22-reproducibility)
23. [Miscellaneous](#23-miscellaneous)

---

## 1. System & Utility

| Command | Description |
|---------|-------------|
| `help` | Prints a quick-reference summary of all available commands. |
| `clear` | Clears the console output and resets the input buffer. |
| `tz show` | Displays the current system timezone offset. |
| `tz set <+HH[:MM]\|-HH[:MM]>` | Sets the system timezone offset (e.g., `tz set +05:30`). |
| `telemetry` | Opens the Telemetry Cockpit GUI window. |

---

## 2. Process & Job Control (POSIX)

| Command | Description |
|---------|-------------|
| `ps` | Lists all running tasks/processes and their states. |
| `kill <pid>` | Terminates the process with the given PID. |
| `nice <pid> <0..7>` | Sets the priority (niceness) of a task. Lower = higher priority. |
| `trace <pid> on` | Enables syscall/execution tracing for a specific process. |
| `trace <pid> off` | Disables tracing for a process. |
| `trace <pid> show` | Displays the current trace log for a process. |
| `jobs` | Lists all background jobs in the shell. |
| `fg <task>` | Brings a background job to the foreground. |
| `bg <task>` | Resumes a stopped job in the background. |
| `tcsetpgrp <tty> <pgid>` | Sets the foreground process group ID for a terminal. |
| `tcgetpgrp <tty>` | Gets the foreground process group ID of a terminal. |

---

## 3. File System & Storage

| Command | Description |
|---------|-------------|
| `ls` | Lists files on the root filesystem (TFS or Ext2). |
| `ls -l` | Lists files in long format showing inode, size, mtime, and name. |
| `stat <path>` | Displays detailed metadata for a file (size, permissions, inode). |
| `delete <path>` | Deletes a file from the virtual filesystem. |
| `mount` | Mounts the root Ext2 filesystem from the AHCI disk. |
| `umount` | Unmounts the currently mounted filesystem. |
| `exec <path>` | Loads and executes a user-mode ELF binary from the given path. |
| `mkdemo` | Creates a demo user-mode binary (`/demo.bin`) on the filesystem for testing `exec`. |
| `save <task_id>` | Saves neural task state to a disk slot. |
| `load <task_id>` | Loads neural task state from a disk slot. |
| `tsave [name]` | Saves the current neural potentials to disk as a named snapshot. |
| `tload <name>` | Loads a previously saved named snapshot from disk. |
| `wipe` | **DESTRUCTIVE** — Erases all snapshot data and resets all neural potentials to zero. |

---

## 4. Page Cache

| Command | Description |
|---------|-------------|
| `pcache` / `pcache show` | Displays page cache hit/miss statistics and disk IO counters. |
| `pcache reset` | Resets all page cache and disk IO statistics. |
| `pcache flush` | Flushes all dirty cache lines to disk. |
| `pcache stress [loops]` | Runs a write-coalescing stress test (default: 1000 loops). |

---

## 5. PCI Bus

| Command | Description |
|---------|-------------|
| `pci` | Scans and lists all devices on the PCI bus. |
| `pci bar` | Locates the AHCI/NVMe storage controller BAR, maps the MMIO region, and prints its address. |

---

## 6. AHCI / SATA Disk Controller

| Command | Description |
|---------|-------------|
| `ahci` / `ahci show` | Prints a detailed AHCI HBA report (ports, capabilities, version). |
| `ahci backend` | Shows the current disk read backend (ATA / AHCI / AUTO). |
| `ahci backend ata` | Forces disk reads through the legacy ATA (PIO) driver. |
| `ahci backend ahci` | Forces disk reads through the native AHCI (DMA) driver. |
| `ahci backend auto` | Lets the kernel auto-select the best disk backend. |
| `ahci reset <port>` | Resets and re-initializes a specific AHCI port. |
| `ahci identify [port]` | Sends an ATA IDENTIFY command to retrieve disk model, serial, firmware, and capacity. |
| `ahci read <lba>` | Reads a 512-byte sector at the given LBA from the first ready port. |
| `ahci read <port> <lba>` | Reads a sector from a specific AHCI port. |
| `ahci smoke <lba>` | Reads the same sector via both ATA and AHCI backends and compares checksums (integrity test). |

---

## 7. USB (xHCI)

| Command | Description |
|---------|-------------|
| `usb` / `usb tree` | Lists all enumerated USB devices with VID:PID, class, port, and driver name. |
| `usb info` | Displays detailed xHCI controller statistics (enumerations, resets, stalls, quarantines). |
| `usb reset all` | Resets all USB ports and re-enumerates all devices. |
| `usb reset <id>` | Resets a specific USB device by its device ID. |
| `usb recover <id>` | Attempts to recover a stalled USB endpoint for a specific device. |
| `usb fault <id> [err]` | Injects a simulated fault into a device for testing error recovery. |
| `usb power suspend` | Suspends all USB ports (power management). |
| `usb power resume` | Resumes all suspended USB ports. |
| `usb hotplug [cycles]` | Runs a hotplug regression test simulating attach/detach cycles (default: 1). |

---

## 8. Audio (AC97) & Sonification

### Audio Playback & Streams
| Command | Description |
|---------|-------------|
| `audio` / `audio list` | Lists all active audio streams with their frequency, volume, QoS, and underrun counts. |
| `audio play [hz] [vol]` | Plays a sine tone at the given frequency and volume (defaults: 440 Hz, 60%). |
| `audio stop [id\|all]` | Stops a specific stream by ID, or stops all streams. |
| `audio stat` | Displays global audio mixer statistics (tick rate, active streams, underruns, overruns). |
| `audio volume <id> <0..100>` | Sets the volume of a specific audio stream. |
| `audio policy <id> <prio> <rate>` | Sets QoS priority (0–3) and rate limit (1–64 samples/tick) for a stream. |

### Audio Capture (Microphone)
| Command | Description |
|---------|-------------|
| `audio cap open [hz] [ch] [bits]` | Opens an audio capture stream (defaults: 16000 Hz, mono, 16-bit). |
| `audio cap read <id> [frames]` | Reads captured audio frames from a capture stream (max 256 frames). |
| `audio cap close <id>` | Closes a capture stream. |

### AC97 Hardware Driver
| Command | Description |
|---------|-------------|
| `audio drv stat` | Displays low-level AC97 hardware report (PCI BDF, format, DMA frames, IRQ count). |
| `audio drv reset [fault]` | Resets the AC97 codec. Append `fault` to inject a simulated reset failure. |
| `audio drv loopback [frames] [hz] [ch] [bits]` | Runs a DMA loopback test and prints CRC (defaults: 1024 frames, 44100 Hz, stereo, 16-bit). |

### Neural Sonification
| Command | Description |
|---------|-------------|
| `sonify` / `sonify stat` | Displays the current sonification status (on/off, base Hz, span Hz). |
| `sonify on [base_hz] [span_hz]` | Enables neural sonification — maps spike rates to audio frequencies (defaults: 220 Hz base, 880 Hz span). |
| `sonify off` | Disables neural sonification. |

---

## 9. Networking (E1000 NIC)

| Command | Description |
|---------|-------------|
| `net` / `net status` | Displays full NIC status: link speed, MAC, IP config, RX stats, TCP stats, fault injection state, coalescing. |
| `net up` | Initializes the E1000 network interface card. |
| `net cfg <ip> <mask> <gw>` | Configures IPv4 addressing (e.g., `net cfg 10.0.2.15 255.255.255.0 10.0.2.2`). |
| `net coalesce <poll> <irqpoll>` | Tunes RX interrupt coalescing budgets. |
| `net fault <loss%> <reorder%>` | Sets fault injection rates for simulating network unreliability. |
| `net tx` | Sends a single test probe packet. |
| `net export <slot>` | Exports a network state snapshot to the given slot. |
| `net manifest` | Exports a network manifest to disk. |
| `net profile` | Exports network performance profile data to disk. |

### TCP
| Command | Description |
|---------|-------------|
| `net tcpserve <port> [loops]` | Starts a TCP echo server on the given port for a number of accept loops. |
| `net tcpdial <ip> <port> [count]` | Connects to a remote TCP endpoint and sends/receives test payloads. |

### UDP
| Command | Description |
|---------|-------------|
| `net udpbench [ip\|self] [count] [bytes]` | Runs a UDP loopback benchmark — sends packets and counts round-trips. |

---

## 10. Remote Access & Authentication

| Command | Description |
|---------|-------------|
| `remote` | Displays remote access status (on/off, token, session, auth validity, role). |
| `remote on` / `remote off` | Enables or disables the remote access subsystem. |
| `remote auth <hex> [ttl]` | Sets the authentication token and TTL (time-to-live in ticks, default: 6000). |
| `remote token <hex>` | Sets the authentication token with default TTL. |
| `remote role <viewer\|operator\|admin>` | Sets the permission role for the remote session. |
| `remote rotate` | Rotates the internal session key. |

---

## 11. Spiking Neural Network (Neuromorphic Engine)

| Command | Description |
|---------|-------------|
| `stim <task_id> [amount]` | Injects a voltage stimulus into a task's neural pixel (default amount: 500). |
| `eval` | Evaluates the overall neural state of Task 0 and reports FLUID/ACTIVE or RIGID/STABLE. |
| `stdp` | Displays whether STDP (Spike-Timing-Dependent Plasticity) is currently enabled. |
| `stdp on` / `stdp off` | Enables or disables STDP unsupervised learning. |
| `neuro caps` | Displays the neuromorphic backend capabilities (max neurons, synapses, plasticity support). |
| `pipeline show` | Displays the current neuromorphic pipeline execution graph topology. |
| `pipeline start` | Starts the neuromorphic pipeline graph. |
| `pipeline stop` | Stops the neuromorphic pipeline graph. |

### Neuron Model Selection
| Command | Description |
|---------|-------------|
| `model` / `model show` | Displays the current neuron model for each task and STDP status. |
| `model select [task] <lif\|adapt\|stdp>` | Selects the neuron model for a specific task. |
| `model param <task> <neuron\|all> <key> <value>` | Sets a parameter on a specific neuron or all neurons in a task. |

---

## 12. AI Runtime & Inference

### Status
| Command | Description |
|---------|-------------|
| `ai` / `ai stat` / `ai show` | Displays AI runtime stats: backend, model count, inference results, QoS, training state. |

### Model Management
| Command | Description |
|---------|-------------|
| `ai model ls` | Lists all loaded AI models with their kind, version, backend, and source path. |
| `ai model load <name> <ann\|snn> [cpu\|neuro] [seed] [version]` | Loads a new model into the runtime registry. |
| `ai model import <path> [alias]` | Imports a model package from the VFS filesystem. |
| `ai model verify <name>` | Verifies a model's integrity and prints its checksum. |

### Quality of Service
| Command | Description |
|---------|-------------|
| `ai qos` / `ai qos show` | Displays the current QoS mode and CPU budget for AI tasks. |
| `ai qos latency [budget_pct]` | Sets QoS to latency-optimized mode with an optional CPU budget percentage. |
| `ai qos throughput [budget_pct]` | Sets QoS to throughput-optimized mode. |

### Inference
| Command | Description |
|---------|-------------|
| `ai infer <model> <input> [seed]` | Runs a single forward-pass inference on the named model with the given input value. |
| `ai bench <model> [rounds] [input] [seed]` | Benchmarks inference latency over N rounds (default: 128). |

### Training
| Command | Description |
|---------|-------------|
| `ai train` / `ai train stat` | Displays training status (model, active, paused, steps, drift alarms). |
| `ai train start <model> [lr_ppm] [max_drift]` | Starts a training loop on the named model. |
| `ai train step [n]` | Runs N training steps (default: 1). |
| `ai train pause` / `ai train resume` | Pauses or resumes an active training loop. |
| `ai train checkpoint` | Creates a checkpoint of the current model weights. |
| `ai train rollback` | Rolls back to the last checkpoint (undo drift). |

### Export
| Command | Description |
|---------|-------------|
| `ai export [path]` | Exports full AI telemetry (runtime + scheduler + training stats) to a file (default: `/ai_telemetry.txt`). |

---

## 13. Profiling & Diagnostics

### Profiling
| Command | Description |
|---------|-------------|
| `profile` / `profile show` | Displays detailed profiling data: render, scheduler, spike, and command cycle times with histograms. |
| `profile on` / `profile off` | Enables or disables kernel-level profiling. |
| `profile reset` | Resets all profiling counters. |
| `profile export [path]` | Exports profiling data to a file (default: `/profile.prf`). |
| `profile hud compact` / `profile hud detail` | Toggles between compact and detailed HUD overlay modes. |
| `profile samples [n]` / `profile stacks [n]` | Displays the N most recent stack samples (default: 8, max: 8). |

### Diagnostics
| Command | Description |
|---------|-------------|
| `diag` / `diag show` | Displays comprehensive diagnostics: display FPS, cursor position, NIC stats, fault injection. |
| `diag cursor` | Shows the current mouse cursor X, Y position and button state. |
| `diag netstress [count]` | Sends N probe packets and measures RX throughput (default: 128). |
| `diag netloss [loss%] [count]` | Temporarily injects packet loss and measures its effect on RX. |
| `diag netreorder [reorder%] [count]` | Temporarily injects packet reordering and measures its effect on UDP. |
| `diag phase24` | Runs the comprehensive Phase 24 integration test (FPS, cursor, NIC, stress, disk). |
| `phasecheck [usb_cycles] [audio_frames]` | Runs automated Phase 30/31 integration tests across USB and audio subsystems. |

### Kernel Debugger
| Command | Description |
|---------|-------------|
| `kdbg show <pid>` | Displays debug info for a specific process. |
| `kdbg stack <pid>` | Prints the call stack (EIP, EBP, return addresses) for a process. |
| `kdbg mem <hexaddr> [len]` | Dumps raw memory at the given address (default: 64 bytes). |
| `kdbg trace <pid> on\|off` | Enables or disables kernel-level execution tracing for a task. |

---

## 14. Kernel Modules

| Command | Description |
|---------|-------------|
| `insmod <path>` | Loads a dynamic ELF kernel module from the given filesystem path. |
| `rmmod <path\|handle>` | Unloads a kernel module by its path or handle. |
| `lsmod` | Lists all currently loaded kernel modules with their handles and entry points. |

---

## 15. POSIX Identity & Permissions

| Command | Description |
|---------|-------------|
| `id [task]` | Displays the UID, GID, and PGID for the current or specified task. |
| `chmod <path> <octal>` | Changes file permissions (e.g., `chmod /demo.bin 755`). |
| `chown <path> <uid> <gid>` | Changes file ownership. |
| `setuid <uid>` | Sets the effective User ID. |
| `setgid <gid>` | Sets the effective Group ID. |
| `setpgid <task> <pgid>` | Sets the Process Group ID for a task. |

---

## 16. Scheduler

| Command | Description |
|---------|-------------|
| `sched` / `sched show` | Displays scheduler metrics: ticks, wake boosts, starvation mitigations, inversion hints, per-band stats. |
| `sched reset` | Resets all scheduler metrics (requires admin). |

---

## 17. Memory

| Command | Description |
|---------|-------------|
| `mall` | Allocates a physical memory page via the PMM and prints its address. |
| `free <hexaddr>` | Frees a previously allocated physical page. |
| `map` | Prints the full physical memory map from the PMM. |
| `memstat` | Displays detailed memory statistics: total/used/free pages, fragmentation, slab allocator stats. |

---

## 18. Visualization (NeuroViz)

| Command | Description |
|---------|-------------|
| `viz` / `viz show` | Displays the current visualization mode, scrub position, and autoplay state. |
| `viz heatmap` | Switches to heatmap visualization mode. |
| `viz raster` | Switches to raster (spike train) visualization mode. |
| `viz off` | Disables visualization. |
| `viz scrub <0..63\|+\|->` | Sets or steps the scrub position in the recording timeline. |
| `viz play on` / `viz play off` | Enables or disables automatic playback of the timeline. |
| `viz compare <a> <b>` | Enters comparison mode showing two snapshot slots side-by-side. |
| `viz export [path]` | Exports a visualization report with snapshot diffs to a file (default: `/viz.rpt`). |

---

## 19. Synapse Inspector

| Command | Description |
|---------|-------------|
| `synview <task:0\|1>` | Displays all synaptic weights, voltages, and thresholds for a task's neurons. |
| `synset <task> <neuron> <weight>` | Manually sets the synaptic weight of a specific neuron. |
| `synrule <task> <neuron> <rule>` | Sets the learning rule for a specific neuron. |
| `synpreset <name>` | Loads a preset synaptic configuration (e.g., excitatory, inhibitory, random). |
| `syncmp <taskA> <taskB>` | Compares the synaptic state of two tasks side-by-side. |

---

## 20. Snapshot Browser

| Command | Description |
|---------|-------------|
| `sbrowse` | Displays an overview of all saved snapshot slots with their signatures. |
| `spreview <slot>` | Shows a detailed preview of a snapshot slot including voltage/weight/threshold signatures and drift state (CONSOLIDATED, PLASTIC-DRIFT, or HIGH-DRIFT). |
| `stag <slot> <tag>` | Tags a snapshot slot with a human-readable label. |
| `sdiff <slotA> <slotB>` | Computes and displays the delta between two snapshot slots with a visual drift bar. |

---

## 21. Data Management

| Command | Description |
|---------|-------------|
| `manifest` / `manifest show` | Displays a summary of the current session manifest. |
| `manifest save [path]` | Saves the session manifest to disk (default: `/session.manifest`). |
| `manifest load [path]` | Loads a session manifest from disk. |
| `dataset export [path]` | Exports neural snapshot data to a file (default: `/snapshot.ds8`). |
| `dataset import [path]` | Imports neural snapshot data from a file. |
| `replay` / `replay show` | Displays all recorded commands in the replay buffer. |
| `replay rec on` / `replay rec off` / `replay rec toggle` | Controls command recording for the replay system. |
| `replay run` | Re-executes all recorded commands in sequence. |
| `replay clear` | Clears the replay buffer. |

---

## 22. Reproducibility

| Command | Description |
|---------|-------------|
| `repro bundle create` | Creates a reproducibility bundle at `/repro.bundle` containing a manifest and model checksums. |
| `repro bundle verify` | Verifies the integrity of an existing reproducibility bundle (checks magic header and manifest). |

---

## 23. Miscellaneous

| Command | Description |
|---------|-------------|
| `zoom+` / `zoom-` | Increases or decreases the neural heatmap zoom level (1x–4x). |
| `zpan+` / `zpan-` | Pans the neural heatmap viewport left or right within the zoomed view. |
| `ipc send <ch> <val>` | Sends a value to an IPC (Inter-Process Communication) channel (requires admin). |
| `ipc recv <ch>` | Receives a value from an IPC channel. |
| `ipc stat <ch>` | Displays the queue depth of an IPC channel. |

---

## Command Count Summary

| Category | Root Commands | Sub-commands |
|----------|:---:|:---:|
| System & Utility | 3 | 5 |
| Process & Job Control | 8 | 14 |
| File System & Storage | 12 | 14 |
| Page Cache | 1 | 4 |
| PCI Bus | 1 | 2 |
| AHCI / SATA | 1 | 10 |
| USB (xHCI) | 1 | 9 |
| Audio (AC97) | 2 | 19 |
| Networking | 1 | 14 |
| Remote Access | 1 | 7 |
| Neuromorphic Engine | 6 | 13 |
| AI Runtime | 1 | 17 |
| Profiling & Diagnostics | 4 | 18 |
| Kernel Modules | 3 | 3 |
| POSIX Identity | 7 | 7 |
| Scheduler | 1 | 2 |
| Memory | 4 | 4 |
| Visualization | 1 | 8 |
| Synapse Inspector | 5 | 5 |
| Snapshot Browser | 4 | 4 |
| Data Management | 4 | 11 |
| Reproducibility | 1 | 2 |
| Miscellaneous | 5 | 7 |
| **TOTAL** | **77** | **~193** |

---

*NeuroSpark OS — Built from scratch in C and x86 Assembly.*
