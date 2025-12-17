#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/time64.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("TSU Lab: Hale-Bopp Comet Tracker");
MODULE_VERSION("1.1");

#define PROC_FILENAME "tsulab"
#define LAST_PERIHELION_TIMESTAMP 859852800ULL
#define ORBITAL_PERIOD_SECONDS 79939261344ULL
#define PRECISION_MULTIPLIER 1000000ULL
#define DIVIDER_FOR_INT 10000ULL

static int tsulab_show(struct seq_file *m, void *v)
{
    u64 current_time = ktime_get_real_seconds();
    u64 elapsed_seconds;
    u64 raw_value;
    u64 integer_part;
    u64 decimal_part;

    if (current_time < LAST_PERIHELION_TIMESTAMP) {
        seq_printf(m, "Error: Current time is before 1997.\n");
        return 0;
    }

    elapsed_seconds = current_time - LAST_PERIHELION_TIMESTAMP;
    raw_value = div64_u64(elapsed_seconds * PRECISION_MULTIPLIER, ORBITAL_PERIOD_SECONDS);
    integer_part = raw_value / DIVIDER_FOR_INT;
    decimal_part = raw_value % DIVIDER_FOR_INT;

    seq_printf(m, "Comet Hale-Bopp Orbit Progress\n");
    seq_printf(m, "Precision: ten-thousandths of a percent\n");
    seq_printf(m, "Progress: %llu.%04llu%%\n", integer_part, decimal_part);

    return 0;
}

static int tsulab_open(struct inode *inode, struct file *file)
{
    return single_open(file, tsulab_show, NULL);
}

static const struct proc_ops tsulab_fops = {
    .proc_open    = tsulab_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init tsulab_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");
    if (!proc_create(PROC_FILENAME, 0444, NULL, &tsulab_fops)) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit tsulab_exit(void)
{
    remove_proc_entry(PROC_FILENAME, NULL);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);