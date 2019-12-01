#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>

MODULE_LICENSE("Dual BSD/GPL");

struct cdev cdev;
struct class *class;

ssize_t
test_write(struct file *file, const char __user *buf, size_t num, loff_t *off)
{
	return num;
}

ssize_t
test_read(struct file *file, char __user *buf, size_t num, loff_t *off)
{
	return num;
}

int test_open(struct inode *inode, struct file *file)
{
	struct page *page;

	page = alloc_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

        file->private_data = page;

        return 0;
}

int test_release(struct inode *inode, struct file *file)
{
	pr_err("PID %d: %s\n", current->pid, __FUNCTION__);
	__free_page(file->private_data);

        return 0;
}

int test_mmap(struct file *file, struct vm_area_struct *vma)
{
        if (remap_pfn_range(vma, vma->vm_start,
			    page_to_pfn((struct page *)file->private_data),
			    PAGE_SIZE, vma->vm_page_prot)) {
                pr_err("fasync: remap_pfn_range failed!\n");
                return -1;
        }

        return 0;
}

int test_flush(struct file *file, fl_owner_t id)
{
	pr_err("PID %d: %s\n", current->pid, __FUNCTION__);
	WARN_ON(1);

        return 0;
}

struct file_operations test_fops = {
        .owner = THIS_MODULE,
        .read = test_read,
        .write = test_write,
        .open = test_open,
        .release = test_release,
	.mmap = test_mmap,
	.flush = test_flush,
};

static int __init fork_init(void)
{
        unsigned int firstminor = 0;
        dev_t dev_id;
        int err = 0;
        unsigned int count = 1;
        char *dev_name = "fork_test";

        /* alloc dev_id */
        err = alloc_chrdev_region(&dev_id, firstminor, count, dev_name);
        if (err < 0) {
                pr_err("fail not allocate a cdev\n");
                return err;
        }

        /* register cdev */
        cdev_init(&cdev, &test_fops);
        err = cdev_add(&cdev, dev_id, count);
        if (err < 0) {
                pr_err("fail not add a cdev to system\n");
                return err;
        }

        class = class_create(THIS_MODULE, dev_name);
        if (IS_ERR(class)) {
                pr_err("fail to create device %s\n", dev_name);
                return -1;
        }

        device_create(class, NULL, dev_id, NULL, "fork" "%d", 0);

        return 0;
}

static void __exit fork_exit(void)
{
        dev_t dev_id = cdev.dev;

        device_destroy(class, dev_id);
        class_destroy(class);
        cdev_del(&cdev);
}

module_init(fork_init);
module_exit(fork_exit);

MODULE_AUTHOR("Sherlock");
MODULE_DESCRIPTION("The driver is for testing fork");
