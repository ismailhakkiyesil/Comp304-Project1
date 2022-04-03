//İsmail Hakkı Yeşil 72293 - Alkan Akısu 71455

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

int pid = 0;
static char *flag;
module_param(pid,int,0);
module_param(flag,charp,0);

//DFS function
void DFS(struct task_struct *ptr)
{   
    struct task_struct *task;
    struct list_head *list;

    printk("Name: %s, PID: %d\n", ptr->comm, ptr->pid);
    list_for_each(list, &ptr->children) {
            if(list_empty(&(ptr->children))){
                printk(KERN_INFO "List empty");
            }else{
                task = list_entry(list, struct task_struct, sibling);
                DFS(task);
            }
            
    }
}

void BFS(struct task_struct *ptr)
{   
    struct task_struct *task;
    struct list_head *list;

    printk("Name: %s, PID: %d\n", ptr->comm, ptr->pid);
    list_for_each(list, &ptr->children) {
            if(list_empty(&(ptr->children))){
                printk(KERN_INFO "List empty");
            }else{
                BFS(task);
                task = list_entry(list, struct task_struct, sibling);
            }
    }
}

int driver_init(void)
{   
    struct pid *pid_struct;
	struct task_struct *task;

    printk(KERN_INFO "Root PID: %d\n",pid);
	
	pid_struct = find_get_pid(pid);
	task = pid_task(pid_struct, PIDTYPE_PID);

    if(task == NULL) {
			printk(KERN_ERR "PID doesn't exist\n");
	}else{
        if(strcmp(flag,"-d")==0){
            DFS(task);
        }else{
            BFS(task);
        }
    }
    return 0;
}

void driver_exit(void)
{
    printk(KERN_INFO "Removing Module\n");
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");