#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sched.h>
#include <signal.h>

#include <sys/stat.h>   
#include <sys/mount.h>  

#define SOCKET_PATH "/tmp/engine_socket"
#define MAX_CONTAINERS 10
#define STACK_SIZE (1024 * 1024)

struct container {
    char id[32];
    pid_t pid;
    char state[16];
};

struct container containers[MAX_CONTAINERS];
int container_count = 0;

char child_stack[STACK_SIZE];


// ---------------- CHILD FUNCTION ----------------
int container_main(void *arg) {
    char **args = (char **)arg;

    char *rootfs = args[0];
    char *cmd = args[1];

    printf("Container starting...\n");

    // Set hostname (UTS isolation)
    if (sethostname("container", strlen("container")) != 0) {
        perror("sethostname failed");
    }

    // Change root
    if (chroot(rootfs) != 0) {
        perror("chroot failed");
        exit(1);
    }

    if (chdir("/") != 0) {
        perror("chdir failed");
        exit(1);
    }

    // Create /proc if not exists
    if (mkdir("/proc", 0555) != 0) {
        // ignore error if already exists
    }

    // Mount /proc
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        perror("mount /proc failed");
    }

    printf("Inside container! PID: %d\n", getpid());

    // Execute command
    char *exec_args[] = {cmd, NULL};
    execvp(cmd, exec_args);

    perror("exec failed");
    return 1;
}


// ---------------- START CONTAINER ----------------
void start_container(char *id, char *rootfs, char *cmd) {
    if (container_count >= MAX_CONTAINERS) {
        printf("Max containers reached\n");
        return;
    }

    char *args[] = {rootfs, cmd, NULL};

    pid_t pid = clone(container_main,
                      child_stack + STACK_SIZE,
                      CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                      args);

    if (pid < 0) {
        perror("clone failed");
        return;
    }

    strcpy(containers[container_count].id, id);
    containers[container_count].pid = pid;
    strcpy(containers[container_count].state, "running");
    container_count++;

    printf("Started container %s (PID %d)\n", id, pid);
}


// ---------------- LIST CONTAINERS ----------------
void list_containers() {
    printf("ID\tPID\tSTATE\n");
    for (int i = 0; i < container_count; i++) {
        printf("%s\t%d\t%s\n",
               containers[i].id,
               containers[i].pid,
               containers[i].state);
    }
}


// ---------------- SUPERVISOR ----------------
void run_supervisor(char *base_rootfs) {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 5);

    printf("Supervisor running...\n");

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);

        char buffer[256] = {0};
        read(client_fd, buffer, sizeof(buffer));

        printf("Received: %s\n", buffer);

        char cmd[32] = {0}, id[32] = {0}, rootfs[128] = {0}, command[128] = {0};

        sscanf(buffer, "%s %s %s %s", cmd, id, rootfs, command);

        if (strcmp(cmd, "start") == 0) {
            start_container(id, rootfs, command);
        } 
        else if (strcmp(cmd, "ps") == 0) {
            list_containers();
        }

        close(client_fd);
    }
}


// ---------------- CLIENT ----------------
void send_command(int argc, char *argv[]) {
    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        exit(1);
    }

    char buffer[256] = {0};

    for (int i = 1; i < argc; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    }

    write(sock, buffer, strlen(buffer));

    close(sock);
}


// ---------------- MAIN ----------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  engine supervisor <rootfs>\n");
        printf("  engine start <id> <rootfs> <cmd>\n");
        printf("  engine ps\n");
        return 1;
    }

    if (strcmp(argv[1], "supervisor") == 0) {
        run_supervisor(argv[2]);
    } else {
        send_command(argc, argv);
    }

    return 0;
}
