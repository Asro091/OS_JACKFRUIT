# Makefile - Build system for container memory monitor project
#
# Builds:
#   - Kernel module (monitor.ko)
#   - User-space programs (engine + workload generators)
#
# Targets:
#   all     -> build everything
#   module  -> build kernel module only
#   ci      -> build user programs without static linking
#   clean   -> remove build artifacts

obj-m += monitor.o

# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Current working directory
PWD := $(shell pwd)

# Linker flags for workload binaries
# Default is static linking (useful for minimal rootfs environments)
WORKLOAD_LDFLAGS ?= -static

# User-space binaries
USER_TARGETS := engine memory_hog cpu_hog io_pulse

# ========================
# Default Targets
# ========================
all: $(USER_TARGETS) module

# CI build (no static linking to avoid toolchain issues)
ci: WORKLOAD_LDFLAGS =
ci: $(USER_TARGETS)

# Build kernel module
module: monitor.ko

# ========================
# User-space Programs
# ========================

engine: engine.c monitor_ioctl.h
	gcc -O2 -Wall -Wextra -o engine engine.c -lpthread

memory_hog: memory_hog.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o memory_hog memory_hog.c

cpu_hog: cpu_hog.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o cpu_hog cpu_hog.c

io_pulse: io_pulse.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o io_pulse io_pulse.c

# ========================
# Kernel Module Build
# ========================
monitor.ko: monitor.c monitor_ioctl.h
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# ========================
# Cleanup
# ========================
clean:
	# Clean kernel module build artifacts
	if [ -d "$(KDIR)" ]; then $(MAKE) -C $(KDIR) M=$(PWD) clean; fi

	# Remove user binaries and intermediate files
	rm -f $(USER_TARGETS) *.o *.mod *.mod.c *.symvers *.order

	# Remove logs and runtime artifacts
	rm -f *.log
	rm -rf logs
	rm -f /tmp/mini_runtime.sock

.PHONY: all ci module clean
