#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int backdoor = 0;

struct node {
    int prio;
    struct node* next;
    struct node* prev;
};

struct prio_list {
    struct node* first;
};

struct prio_list zlist;

void add_to_tail(struct node* new_node, struct node* node_after) {
    struct node* node_before = node_after->prev;
    new_node->next = node_after;
    new_node->prev = node_before;
    node_before->next = new_node;
    node_after->prev = new_node;
}

void add_node(struct node* new_node) {
    struct node* cur = zlist.first;
    struct node* node_before = zlist.first->prev;  //last
    bool new_node_the_largest = 1;
    do {
        if(cur->prio > new_node->prio) {
            node_before = cur->prev;
            new_node_the_largest = 0;
            break;
        }
        cur = cur->next; 
    }
    while(cur != zlist.first);

    add_to_tail(new_node, cur);
    if(cur == zlist.first && new_node_the_largest == 0) {  // new node needs to be inserted in the front
        zlist.first = new_node;
    }
}

struct node* alloc(int prio)
{
    struct node* new_node = malloc(sizeof(struct node));
    new_node->prio = prio;
    return new_node;
}

void main(int argc, char** argv) {
    struct node* a_node = malloc(sizeof(struct node));
    a_node->prio = 0;
    a_node->prev = a_node;
    a_node->next = a_node;
    zlist.first = a_node;

    int opt;
    printf("Option menu:\n[1] add a node;\n[2] changing the prev pointer of the last node;\n[3] debugging feature (disabled);\n[4] exit;\n");
    while(1){
        printf("Input option: ");
        scanf("%d", &opt);
        char prev_str[20];
        int prev;
        switch(opt){
            case 1:
                printf("Input priority of the new node: ");
                int prio;
                scanf("%d", &prio);
                struct node* new_node = alloc(prio);
                add_node(new_node);
		printf("Address: %p\n", new_node);
                break;
            case 2:
                printf("Input the new prev pointer of the last node: ");
                scanf("%10s", prev_str);
                prev = (int)strtol(prev_str, NULL, 0);
                printf("prev pointer: %x\n", prev);
                zlist.first->prev->prev = (struct node*)prev;
                break;
            case 3:
                printf("backdoor: %d\n", backdoor);
                if(backdoor) {
                    printf("debugging feature is enabled, will give a root shell\n");
                    execve("/bin/sh", NULL, NULL);
                }
                else {
                    printf("I'm sorry this feature is disabled\n");
                }
                break;
            case 4:
                return;
            default:
                break;
        }   
    }   


}
