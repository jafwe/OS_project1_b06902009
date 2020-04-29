#include<linux/linkage.h>
#include<linux/kernel.h>
#include<linux/ktime.h>

asmlinkage int sys_my_dmesg(char *my_dmesg)
{
	printk(KERN_INFO);
	printk(my_dmesg, "\n");
	//printk("my_dmesg is invoked!\n");
	return 0;
}
