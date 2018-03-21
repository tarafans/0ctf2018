#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "zerofs.h"

static DEFINE_MUTEX(zerofs_sb_lock);
static DEFINE_MUTEX(zerofs_inodes_lock);
static DEFINE_MUTEX(zerofs_dir_lock);

static int zerofs_init(void)
{
	dbg_printf("Hello zerofs!\n");
	return 0;
}

static void zerofs_exit(void)
{
	dbg_printf("Byebye zerofs!\n");
}

module_init(zerofs_init);
module_exit(zerofs_exit);

MODULE_LICENSE("GPL");

