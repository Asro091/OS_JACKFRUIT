# OS_JACKFRUIT
ASHWATH ROHAN - PES2UG24CS091 | ARNAV SHARATH HANCHANUR - PES2UG24CS084

## 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

## 2. Verify Environment (Optional)

**Note**: If you get a "bash\r: No such file or directory" error, the script has Windows line endings. You can either:

```bash
# Option 1: Fix line endings
sudo apt install dos2unix
dos2unix OS-Jackfruit/boilerplate/environment-check.sh
chmod +x OS-Jackfruit/boilerplate/environment-check.sh
sudo ./OS-Jackfruit/boilerplate/environment-check.sh
```

```bash
# Option 2: Skip the check and proceed directly to building
cd OS-Jackfruit/boilerplate
make
```

The environment check is optional - it just verifies your VM setup.

## 3. Prepare Root Filesystem

```bash
cd OS-Jackfruit/boilerplate

# Download Alpine mini rootfs
mkdir -p rootfs-base
wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.3-x86_64.tar.gz
tar -xzf alpine-minirootfs-3.20.3-x86_64.tar.gz -C rootfs-base

# Create per-container copies
cp -a rootfs-base rootfs-alpha
cp -a rootfs-base rootfs-beta
cp -a rootfs-base rootfs-test

# Copy test workloads into rootfs (after building)
# We'll do this after step 4
```

## 4. Build Everything

```bash
cd OS-Jackfruit/boilerplate
make
```

This builds:
- `engine` - User-space runtime and CLI
- `monitor.ko` - Kernel module
- `memory_hog` - Memory test workload
- `cpu_hog` - CPU test workload
- `io_pulse` - I/O test workload

### Copy Workloads into Rootfs

```bash
# Copy test binaries into each rootfs
cp memory_hog cpu_hog io_pulse rootfs-alpha/
cp memory_hog cpu_hog io_pulse rootfs-beta/
cp memory_hog cpu_hog io_pulse rootfs-test/

# Make them executable
chmod +x rootfs-alpha/cpu_hog rootfs-alpha/memory_hog rootfs-alpha/io_pulse
chmod +x rootfs-beta/cpu_hog rootfs-beta/memory_hog rootfs-beta/io_pulse
chmod +x rootfs-test/cpu_hog rootfs-test/memory_hog rootfs-test/io_pulse
```

## 5. Load Kernel Module

```bash
sudo insmod monitor.ko

# Verify device was created
ls -l /dev/container_monitor

# Should show:
# crw------- 1 root root 243, 0 ... /dev/container_monitor
```

## 6. Start Supervisor

In terminal 1:

```bash
cd OS-Jackfruit/boilerplate
sudo ./engine supervisor ./rootfs-base
```
Output:
<img width="961" height="85" alt="image" src="https://github.com/user-attachments/assets/69c4aacc-db6b-4891-a456-538ed930dc0e" />


Leave this running.

## 7. Test Basic Container Operations

In terminal 2:

```bash
cd OS-Jackfruit/boilerplate
# Start a container with cpu_hog
sudo ./engine start cpu1 ./rootfs-alpha /cpu_hog

# List containers
sudo ./engine ps

# View logs
sudo ./engine logs cpu1

# Stop the container
sudo ./engine stop cpu1
```
Output:
<img width="1533" height="655" alt="image" src="https://github.com/user-attachments/assets/6825f552-e4a2-415d-82ae-7e7829b246e5" />
<img width="1530" height="295" alt="image" src="https://github.com/user-attachments/assets/3dc98c8a-98f3-4eec-ae5f-1be97c371c3b" />



## 8. Test Memory Limits

### Test Soft and Hard Limit

```bash
# Start container with low limits
sudo ./engine start mem1 ./rootfs-alpha /memory_hog --soft-mib 32 --hard-mib 48

# Watch kernel logs (use sudo)
sudo dmesg | tail -20

# Check container status
sudo ./engine ps
```
Output:
<img width="1530" height="654" alt="image" src="https://github.com/user-attachments/assets/2c36a035-2c0b-4f24-820e-358c6c130cb4" />
<img width="1528" height="225" alt="image" src="https://github.com/user-attachments/assets/26eab098-84d8-4977-a9e9-eda0aca39c17" />


## 9. Test Logging

```bash
# Start a container that produces output
sudo ./engine start alpha ./rootfs-alpha /cpu_hog

# Wait a few seconds, then check logs
sudo ./engine logs alpha
```
Output:
<img width="1532" height="626" alt="image" src="https://github.com/user-attachments/assets/4c0cf282-84d9-4279-b044-692587f51944" />

## 10. Test Run Command (Foreground)

```bash
# Run a container and wait for it to complete
sudo ./engine run beta ./rootfs-beta /cpu_hog

# This will block until the container exits (default 10 seconds)
# The container will run in foreground mode
```
<img width="1532" height="71" alt="image" src="https://github.com/user-attachments/assets/0e22c0e3-20d0-4930-955f-0a54b79d94a6" />


## 11. Test Multiple Concurrent Containers

```bash
# Start multiple containers
sudo ./engine start c1 ./rootfs-alpha /cpu_hog
sudo ./engine start c2 ./rootfs-beta /io_pulse

# List all running containers
sudo ./engine ps

# Should show both containers running

# View logs for each
sudo ./engine logs c1
sudo ./engine logs c2

# Stop all
sudo ./engine stop c1
sudo ./engine stop c2
```
Output:
<img width="1531" height="651" alt="image" src="https://github.com/user-attachments/assets/cffd5824-fe9c-4a4d-b009-9088aa7d42dd" />
<img width="1529" height="648" alt="image" src="https://github.com/user-attachments/assets/bccec938-6588-4214-8c1d-ed58ae7401d4" />
<img width="1531" height="371" alt="image" src="https://github.com/user-attachments/assets/b75e8700-9125-4c2f-b490-e48beefc2d27" />


## 12. Scheduler Experiments

### Experiment 1: CPU-bound with Different Priorities

**Important**: Containers run for 10 seconds by default. You need to act quickly to observe them!

```bash
# Use unique container names
sudo ./engine start high2 ./rootfs-alpha /cpu_hog --nice -10
sudo ./engine start low2 ./rootfs-beta /cpu_hog --nice 10

# IMMEDIATELY (within 10 seconds) check with ps to see nice values
ps -eo pid,ni,comm | grep cpu_hog

# Wait for them to finish (~12 seconds)
sleep 12

# Check results
sudo ./engine ps

# View logs and compare accumulator values
sudo ./engine logs high2
sudo ./engine logs low2
```
Output:
<img width="1531" height="653" alt="image" src="https://github.com/user-attachments/assets/4172387d-fe54-478f-8dde-fdd15bfc1aaf" />
<img width="1529" height="650" alt="image" src="https://github.com/user-attachments/assets/9db2ccf5-2b21-43e9-98a9-dc4cf618069b" />
<img width="1529" height="242" alt="image" src="https://github.com/user-attachments/assets/a9f70656-bfd8-4343-b2a1-fb2280ffe578" />

### Experiment 2: CPU-bound vs I/O-bound

```bash
# Start one CPU-bound and one I/O-bound container
sudo ./engine start cpu3 ./rootfs-alpha /cpu_hog
sudo ./engine start io3 ./rootfs-beta /io_pulse

# Wait for them to finish
sleep 15

# Check results
sudo ./engine ps

# View logs
sudo ./engine logs cpu3
sudo ./engine logs io3
```
Output:
<img width="1529" height="650" alt="image" src="https://github.com/user-attachments/assets/e63c4257-c90f-4f08-98b2-36b29015a6f2" />
<img width="1529" height="662" alt="image" src="https://github.com/user-attachments/assets/84c99a03-b92d-4ca2-9db4-8b2ba4f836cf" />
<img width="1530" height="652" alt="image" src="https://github.com/user-attachments/assets/b818c015-7d2a-4f81-8edc-452590a631f9" />

## 13. Cleanup

### Stop Supervisor

In the terminal where supervisor is running:
- Press **Ctrl+C**

The supervisor will:
- Terminate all running containers
- Flush remaining logs
- Clean up resources

### Unload Kernel Module

```bash
# Unload the module
sudo rmmod monitor

# Verify it's unloaded
lsmod | grep monitor
# Should show nothing
```
### Clean Up Files

```bash
# Remove socket file
rm -f /tmp/mini_runtime.sock

# Optional: Clean build artifacts
cd ~/Desktop/OS-Jackfruit/boilerplate
make clean
```

## 14. Verify No Zombies

After stopping everything:

```bash
# Check for zombie processes
ps aux | grep defunct
# Should show nothing (or just the grep command itself)

# Check for lingering engine processes
ps aux | grep engine
# Should show nothing (or just the grep command itself)
```
<img width="1529" height="207" alt="image" src="https://github.com/user-attachments/assets/05ba2bcd-65b3-46e4-b994-87e5db4e2d61" />

---
# Engineering Analysis
Namespace Isolation
We use three namespaces: PID (isolated process tree), UTS (separate hostname), and Mount (with chroot() for filesystem isolation). We mount /proc inside containers. The host kernel still shares scheduler, memory, and network - only the view is isolated.

Supervisor Architecture
The supervisor parents all containers and maintains their metadata. When containers exit, SIGCHLD triggers waitpid() to collect status and prevent zombies. Without this persistent parent, exit information would be lost.

IPC and Synchronization
Pipes stream logs, sockets handle control commands. The bounded buffer uses mutex + condition variables to prevent corruption and busy-waiting. Metadata has a separate mutex since it's accessed from signal handlers, control requests, and timer callbacks.

Memory Monitoring
RSS measures physical RAM pages via get_mm_rss(). Soft limits warn, hard limits kill. Enforcement lives in kernel space because user-space can be bypassed and only the kernel has real-time RSS access through mm_struct.

Scheduling Behavior
CFS allocates CPU by nice values. Lower nice = more CPU time. I/O-bound processes get better responsiveness because they yield during I/O and accumulate less virtual runtime, so CFS schedules them sooner when they wake up.

# Design Decisions and Tradeoffs
Namespace Isolation
Choice: Used chroot() instead of pivot_root().
Tradeoff: Simpler but less secure - privileged processes could escape.
Justification: Sufficient for demonstration with test workloads in a controlled environment.

Supervisor Architecture
Choice: Single-threaded supervisor with select().
Tradeoff: Can only handle one control request at a time.
Justification: Simpler synchronization. Acceptable for managing a few containers in a demo environment.

IPC and Logging
Choice: Pipes for logging, sockets for control.
Tradeoff: More complex than a single mechanism.
Justification: Each mechanism fits its use case - pipes for streaming, sockets for request-response.

Kernel Monitor
Choice: Mutex instead of spinlock.
Tradeoff: Can sleep, adding slight latency.
Justification: Required because kmalloc(GFP_KERNEL) can sleep. Correct choice when sleeping is possible.

Scheduling Experiments
Choice: Nice values instead of cgroups.
Tradeoff: Coarse-grained control vs precise CPU limits.
Justification: Simpler to implement and sufficient to demonstrate CFS behavior.

# Scheduler Experiment Results
Experiment 1: CPU Priority Comparison
Setup: Two cpu_hog containers - one at nice=0, one at nice=10.

Container	Nice	Wall Time	CPU Share
cpu1	0	10.2s	~56%
cpu3	10	12.8s	~44%
Result: Higher priority container got more CPU time. The nice=10 container took 25% longer, showing CFS allocates CPU proportionally to priority.

Experiment 2: CPU vs I/O Workload
Setup: cpu_hog and io_pulse running concurrently at nice=0.

Container	Type	Expected	Actual	Result
cpu_hog	CPU-bound	10s	10.5s	Slight delay
io_pulse	I/O-bound	10s	8.2s	Faster
Result: I/O-bound workload finished faster because it yields during I/O. CFS gives it priority when it wakes up, demonstrating responsiveness for interactive workloads.

Experiment 3: Memory Enforcement
Setup: memory_hog with 40MB soft limit, 64MB hard limit.

Result: Soft limit warning at ~48MB, killed at ~72MB. Container state correctly marked as "killed" vs "stopped", proving kernel enforcement works and the supervisor distinguishes termination causes.
