/*
 * monitor_ioctl.h - IOCTL interface for container memory monitor
 *
 * Defines:
 *   - User ↔ Kernel communication structure
 *   - IOCTL commands for registering and unregistering processes
 *
 * This header is shared between kernel space and user space.
 */

#ifndef MONITOR_IOCTL_H
#define MONITOR_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

/* Maximum length for container identifier string */
#define MONITOR_NAME_LEN 32

/* =========================================
 * Request Structure
 * =========================================
 * Used by user-space to communicate with the kernel module.
 */
struct monitor_request {
    pid_t pid;                          // Target process ID
    unsigned long soft_limit_bytes;     // Soft memory limit (bytes)
    unsigned long hard_limit_bytes;     // Hard memory limit (bytes)
    char container_id[MONITOR_NAME_LEN]; // Logical container identifier
};

/* =========================================
 * IOCTL Definitions
 * ========================================= */
#define MONITOR_MAGIC 'M'

/*
 * Register a process for monitoring.
 * Expects a populated monitor_request struct.
 */
#define MONITOR_REGISTER _IOW(MONITOR_MAGIC, 1, struct monitor_request)

/*
 * Unregister a process from monitoring.
 * Identified using pid + container_id.
 */
#define MONITOR_UNREGISTER _IOW(MONITOR_MAGIC, 2, struct monitor_request)

#endif /* MONITOR_IOCTL_H */
