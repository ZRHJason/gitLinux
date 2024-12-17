

#define DEVICE_NAME "scull"   // 设备名称
#define BUFFER_SIZE 1024      // 缓冲区大小

static int scull_major;       // 主设备号
static char *device_buffer;   // 设备数据缓冲区
static struct cdev scull_cdev;

static int scull_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "SCULL: Device opened\n");
    return 0;
}

static int scull_release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "SCULL: Device closed\n");
    return 0;
}

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    size_t available = BUFFER_SIZE - *f_pos;  // 剩余可读字节数
    size_t bytes_to_read = min(count, available);

    if (*f_pos >= BUFFER_SIZE) return 0;  // 到达文件末尾
    if (copy_to_user(buf, device_buffer + *f_pos, bytes_to_read)) return -EFAULT;

    *f_pos += bytes_to_read;  // 更新文件指针
    printk(KERN_INFO "SCULL: Read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    size_t available = BUFFER_SIZE - *f_pos;  // 剩余可写字节数
    size_t bytes_to_write = min(count, available);

    if (*f_pos >= BUFFER_SIZE) return -ENOMEM;  // 超出缓冲区
    if (copy_from_user(device_buffer + *f_pos, buf, bytes_to_write)) return -EFAULT;

    *f_pos += bytes_to_write;  // 更新文件指针
    printk(KERN_INFO "SCULL: Wrote %zu bytes\n", bytes_to_write);
    return bytes_to_write;
}

static loff_t scull_llseek(struct file *filp, loff_t offset, int whence) {
    loff_t new_pos = 0;

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = filp->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = BUFFER_SIZE + offset;
        break;
    default:
        return -EINVAL;
    }

    if (new_pos < 0 || new_pos > BUFFER_SIZE) return -EINVAL;
    filp->f_pos = new_pos;
    return new_pos;
}

static struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
    .llseek = scull_llseek,
};

static int __init scull_init(void) {
    dev_t dev;
    int result;

    // 动态分配主设备号
    result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (result < 0) {
        printk(KERN_WARNING "SCULL: Can't allocate major number\n");
        return result;
    }
    scull_major = MAJOR(dev);

    // 初始化缓冲区
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }
    memset(device_buffer, 0, BUFFER_SIZE);

    // 初始化字符设备
    cdev_init(&scull_cdev, &scull_fops);
    scull_cdev.owner = THIS_MODULE;
    result = cdev_add(&scull_cdev, dev, 1);
    if (result) {
        kfree(device_buffer);
        unregister_chrdev_region(dev, 1);
        return result;
    }

    printk(KERN_INFO "SCULL: Initialized with major number %d\n", scull_major);
    return 0;
}

static void __exit scull_exit(void) {
    cdev_del(&scull_cdev);
    kfree(device_buffer);
    unregister_chrdev_region(MKDEV(scull_major, 0), 1);
    printk(KERN_INFO "SCULL: Unloaded\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Basic SCULL Character Device Driver");
