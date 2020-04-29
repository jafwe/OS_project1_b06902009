#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>

asmlinkage long sys_my_time(long *tvsec, long *tvnsec)
{
	struct timespec ts;
	const long n1e9 = 1e9;
	long tvsecnsec;
	getnstimeofday(&ts);
	//printk("%d %d \n",ts.tv_sec,ts.tv_nsec);
	*tvsec = ts.tv_sec;
	*tvnsec = ts.tv_nsec;
	tvsecnsec = ts.tv_sec*n1e9 + ts.tv_nsec;
	//printk("my_time is invoked!\n");
	return tvsecnsec;
}
