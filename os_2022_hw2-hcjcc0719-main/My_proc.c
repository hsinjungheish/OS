#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/pid.h>
#include <linux/proc_fs.h>


#define PROCFS_NAME "thread_info" 
#define PROCFS_MAX_SIZE 2048
 
/* This structure hold information about the /proc file */ 
static struct proc_dir_entry *our_proc_file; 
 
/* The buffer used to store character for this module */ 
static char procfs_buffer[PROCFS_MAX_SIZE]; 
 
/* The size of the buffer */ 
static unsigned long procfs_buffer_size = 0; 
 
static long int num;
static char data_buffer[PROCFS_MAX_SIZE];
static char utime_str[1024];
static char context_str[1024];


static struct task_struct *task;

static ssize_t procfile_read(struct file *filePointer, char *buffer, size_t length, loff_t * offset)
{

	if (*offset || procfs_buffer_size == 0) {
		printk(KERN_INFO "procfs_read: END\n");
		*offset = 0;
		procfs_buffer_size = 0;
		return 0;
	}
	
	strcpy(procfs_buffer, data_buffer);
	procfs_buffer_size = min(strlen(data_buffer), length);

	if ( copy_to_user(buffer, procfs_buffer, procfs_buffer_size) ) {
		return -EFAULT;
	}

	strcpy(data_buffer, "\0");

	*offset += procfs_buffer_size;
	procfs_buffer_size = 0;

	return procfs_buffer_size;	
}


static ssize_t procfile_write(struct file *file, const char *buffer, size_t len, loff_t *off)
{
	if(procfs_buffer_size == 0)
	{
		if ( len > PROCFS_MAX_SIZE )	{
		procfs_buffer_size = PROCFS_MAX_SIZE + 1;
		}
		else	{
			procfs_buffer_size = len + 1;
		}

		if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
			return -EFAULT;
		}
		
		kstrtol(procfs_buffer, 10, &num);

		strcat(data_buffer, "PID:");
		strcat(data_buffer, procfs_buffer);
		strcat(data_buffer, "\n");

		*off += procfs_buffer_size;
		
	}
	else{
		if ( len > PROCFS_MAX_SIZE )	{
		procfs_buffer_size = PROCFS_MAX_SIZE + 1;
		}
		else	{
			procfs_buffer_size = len + 1;
		}

		if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
			return -EFAULT;
		}
		
		kstrtol(procfs_buffer, 10, &num);
		task = get_pid_task(find_get_pid(num), PIDTYPE_PID);

		strcat(data_buffer, "\tThreadID:");
		strcat(data_buffer, procfs_buffer);
		sprintf(utime_str, "%lld", (task->utime / 1000000));
		sprintf(context_str, "%ld", (task->nvcsw + task->nivcsw));
		strcat(data_buffer, " Time:");
		strcat(data_buffer, utime_str);
		strcat(data_buffer, "(ms) ");	
		strcat(data_buffer, "context switch times:");
		strcat(data_buffer, context_str);
		strcat(data_buffer, "\n");

		*off += procfs_buffer_size;
	}

	return procfs_buffer_size;
}

static const struct proc_ops proc_file_fops = { 
    .proc_read = procfile_read, 
    .proc_write = procfile_write, 
}; 

static int __init procfs2_init(void) 
{ 
    our_proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_fops); 
    if (NULL == our_proc_file) { 
        proc_remove(our_proc_file); 
        pr_alert("Error:Could not initialize /proc/%s\n", PROCFS_NAME); 
        return -ENOMEM; 
    } 
 
    pr_info("/proc/%s created\n", PROCFS_NAME); 
    return 0; 
} 
 
static void __exit procfs2_exit(void) 
{ 
    proc_remove(our_proc_file); 
    pr_info("/proc/%s removed\n", PROCFS_NAME); 
} 
 
module_init(procfs2_init); 
module_exit(procfs2_exit); 
 
MODULE_LICENSE("GPL");
