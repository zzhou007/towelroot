#include <sys/mman.h>
#include <unistd.h>
 #include <sys/types.h>

//void add_to_tail(void* preferred_addr, int data, void* next, )

typedef struct
{
    int data;
    void (*fp)(int);
}obj1;

typedef struct 
{
    void (*fp)(int);
    int data;
}obj2;

//typedef struct obj2 my_obj;
obj1* obj_ptr1;
obj2* obj_ptr2;
obj2* last_obj_ptr;

void print_data(int data)
{
    printf("cur_data: %d\n", data);
}

void back_door(int data)
{   
    char* arg[]={"/system/bin/sh",NULL};
    execv("/system/bin/sh",arg);
}

obj2* alloc_a(int data) {
    obj_ptr2 = malloc(sizeof(obj2));
    obj_ptr2->data = data;
    obj_ptr2->fp = &print_data;
    return obj_ptr2;
}

obj2* alloc_b(int data) {
    obj_ptr1 = malloc(sizeof(obj1));
    obj_ptr1->data = data;
    obj_ptr1->fp = &print_data;
    return obj_ptr1;
}

void call_func_ptr(obj2* local_obj_ptr)
{
    printf("data: %d %p\n",local_obj_ptr->data, local_obj_ptr->fp);
    (*(local_obj_ptr->fp))(local_obj_ptr->data);
}

int main(int argc, char** args){
    int opt;
    int data;
    obj2* local_obj_ptr_a;
    obj2* local_obj_ptr_b;
    printf("Option menu:\n[1] create a node_a;\n[2] create a node_b;\n[3] free a node_a;\n[4] free a node_b;\n[5] print node_a data;\n[6] print node_b data;\n[7] exit;\n");
    printf("back_door addr: %p\n", &back_door);
    while(1){
        printf("Input option: ");
        scanf("%d", &opt);
        switch(opt){
        case 1:
            printf("Input data: ");
            scanf("%d", &data);
            local_obj_ptr_a = alloc_a(data);
            break;
        case 2:
            printf("Input data: ");
            scanf("%d", &data);
            local_obj_ptr_b = alloc_b(data);
            break;
        case 3:
            free(local_obj_ptr_a);
            break;
        case 4:
            free(local_obj_ptr_b);
            break;
        case 5:
            call_func_ptr(local_obj_ptr_a);
            break;
        case 6:
            call_func_ptr(local_obj_ptr_b);
            break;
        case 7:
            if(argc == 0) {
                back_door(0);
            }
            return 0;
        default:
            break;
        }
    }
    return 0;
}

