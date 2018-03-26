#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "zerofs.h"

static DEFINE_MUTEX(zerofs_sb_lock);
static DEFINE_MUTEX(zerofs_inode_lock);
static DEFINE_MUTEX(zerofs_dir_lock);

static struct kmem_cache *zerofs_inode_cachep;

// mount 
static struct dentry *zerofs_mount(struct file_system_type *fs_type,
				     int flags, const char *dev_name,
				     void *data);

// super_block
static void zerofs_sync_sb(struct super_block *sb);
static int zerofs_fill_super(
	struct super_block *sb, void *data, int silent);
static void zerofs_kill_superblock(struct super_block *sb);

// inode
static struct zerofs_inode *zerofs_find_inode(
	struct super_block *sb, 
	struct zerofs_inode *start, 
	struct zerofs_inode *target);
static struct zerofs_inode *zerofs_get_inode(
	struct super_block *sb, uint64_t ino);
static int zerofs_sync_inode(
	struct super_block *sb, 
	struct zerofs_inode *zfs_inode);
static void zerofs_add_inode(
	struct super_block *sb, 
	struct zerofs_inode *inode);
static void zerofs_destroy_inode(struct inode *inode);

// data block
static int zerofs_get_datablock(
	struct super_block *sb, uint64_t *dno);

// inode operations
static int zerofs_mkdir(
	struct inode *dir, 
	struct dentry *dentry, 
	umode_t mode);
static int zerofs_create(
	struct inode *dir, 
	struct dentry *dentry, 
	umode_t mode, 
	bool excl);
static struct dentry *zerofs_lookup(
	struct inode *dir, 
	struct dentry *dentry, 
	unsigned int flags);

// dir operations
static int zerofs_iterate(
	struct file *filp, 
	struct dir_context *ctx);

// file operations
static ssize_t zerofs_read(
	struct file *filp, 
	char __user *buf, 
	size_t len, 
	loff_t *ppos);
static ssize_t zerofs_write(
	struct file *filp, 
	const char __user *buf, 
	size_t len, 
	loff_t *ppos);

// helper
static int zerofs_create_dir_or_file(
	struct inode *dir, 
	struct dentry *dentry, 
	umode_t mode);

static struct inode *zerofs_iget(
	struct super_block *sb, 
	uint64_t ino);



static void zerofs_sync_sb(struct super_block *sb)
{
	struct buffer_head *bh = NULL;
	struct zerofs_super_block *zfs_sb = sb->s_fs_info;

	bh = sb_bread(sb, ZEROFS_SB_BLKNO);
	BUG_ON(!bh);

	bh->b_data = (char *)zfs_sb;
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
}

static struct zerofs_inode *zerofs_find_inode(struct super_block *sb, struct zerofs_inode *start, struct zerofs_inode *target)
{
	uint64_t count = 0;

	while (start->ino != target->ino 
			&& count < ((struct zerofs_super_block *)sb->s_fs_info)->inode_count) {
		count++;
		start++;
	}

	if (start->ino == target->ino)
		return start;

	return NULL;
}

static struct zerofs_inode *zerofs_get_inode(struct super_block *sb, uint64_t ino)
{
	struct zerofs_super_block *zfs_sb = sb->s_fs_info;
	struct zerofs_inode *zfs_inode_iter;
	struct zerofs_inode *ret = NULL;
	struct buffer_head *bh = NULL;

	int i;

	bh = sb_bread(sb, ZEROFS_IMAP_BLKNO);
	BUG_ON(!bh);	

	zfs_inode_iter = (struct zerofs_inode *)bh->b_data;

	for (i = 0; i < zfs_sb->inode_count; i++) {
		if (zfs_inode_iter->ino == ino) {
			ret = (struct zerofs_inode *)kmem_cache_alloc(zerofs_inode_cachep, GFP_KERNEL);
			memcpy((void *)ret, (void *)zfs_inode_iter, sizeof(struct zerofs_inode));
			break;
		}
		zfs_inode_iter++;
	}

	brelse(bh);
	return ret;
}

static int zerofs_get_datablock(struct super_block *sb, uint64_t *dno) 
{
	struct zerofs_super_block *zfs_sb = sb->s_fs_info;
	uint64_t i;

	if (mutex_lock_interruptible(&zerofs_sb_lock)) {
		dbg_printf("Fail to acquire lock when getting free data block.\n");
		return -EINTR;
	}

	for (i = 3; i < ZEROFS_MAX_FS_OBJECTS; i++) {
		if (zfs_sb->free_blocks & (1 << i)) 
			break;
	}

	if (i == ZEROFS_MAX_FS_OBJECTS) {
		dbg_printf("No more free data block.\n");
		mutex_unlock(&zerofs_sb_lock);
		return -ENOSPC;
	}

	*dno = i;
	zfs_sb->free_blocks &= ~(1 << i);

	zerofs_sync_sb(sb);

	mutex_unlock(&zerofs_sb_lock);
	return 0;
}

static int zerofs_sync_inode(struct super_block *sb, struct zerofs_inode *zfs_inode)
{
	struct zerofs_inode *_zfs_inode;
	struct buffer_head *bh = NULL;	

	bh = sb_bread(sb, ZEROFS_IMAP_BLKNO);
	BUG_ON(!bh);

	if (mutex_lock_interruptible(&zerofs_sb_lock)) {
		dbg_printf("Fail to acquire lock when syncing inode.\n");
		return -EINTR;
	}

	_zfs_inode = zerofs_find_inode(sb, (struct zerofs_inode *)bh->b_data, zfs_inode);

	if (_zfs_inode) {
		memcpy(_zfs_inode, zfs_inode, sizeof(struct zerofs_inode));
		dbg_printf("Inode %lld updated.\n", _zfs_inode->ino);
		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
	} else {
		mutex_unlock(&zerofs_sb_lock);
		dbg_printf("Fail to update inode %lld (inode missing?).\n", zfs_inode->ino);
		return -EIO;
	}

	brelse(bh);
	
	mutex_unlock(&zerofs_sb_lock);

	return 0;
}

static void zerofs_add_inode(struct super_block *sb, struct zerofs_inode *inode)
{
	struct zerofs_super_block *zfs_sb = sb->s_fs_info;
	struct zerofs_inode *zfs_inode;
	struct buffer_head *bh = NULL;

	if (mutex_lock_interruptible(&zerofs_inode_lock)) {
		dbg_printf("Failed to acquire lock when adding a zerofs inode.\n");
		return;
	}

	bh = sb_bread(sb, ZEROFS_IMAP_BLKNO);
	BUG_ON(!bh);

	zfs_inode = (struct zerofs_inode *)bh->b_data;
	if (mutex_lock_interruptible(&zerofs_sb_lock)) {
		dbg_printf("Failed to acquire lock when adding a zerofs inode.\n");
		return;
	}

	zfs_inode += zfs_sb->inode_count;
	memcpy(zfs_inode, inode, sizeof(struct zerofs_inode));
	zfs_sb->inode_count++;

	mark_buffer_dirty(bh);
	zerofs_sync_sb(sb);
	brelse(bh);

	mutex_unlock(&zerofs_sb_lock);
	mutex_unlock(&zerofs_inode_lock);
}

static int zerofs_iterate(struct file *filp, struct dir_context *ctx)
{
	struct inode *inode = filp->f_inode;
	struct super_block *sb = inode->i_sb;
	struct zerofs_inode *zfs_inode = inode->i_private;
	struct zerofs_dir_record *zfs_dir_rec;
	struct buffer_head *bh = NULL;
	int i;
	
	if (ctx->pos)
		return 0;

	if (!S_ISDIR(zfs_inode->mode)) {
		dbg_printf("Try to iterate non-directory.\n");
		return -ENOTDIR;
	}

	bh = sb_bread(sb, zfs_inode->dno);
	BUG_ON(!bh);

	zfs_dir_rec = (struct zerofs_dir_record *)bh->b_data;	
	for (i = 0; i < zfs_inode->children_count; i++) {
		dir_emit(ctx, zfs_dir_rec->filename, ZEROFS_NAME_LEN, zfs_dir_rec->ino, DT_UNKNOWN);
		ctx->pos += sizeof(struct zerofs_dir_record);
		zfs_dir_rec++;
	}

	brelse(bh);

	return 0;
}

static ssize_t zerofs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos) 
{
	struct zerofs_inode *zfs_inode = filp->f_path.dentry->d_inode->i_private;
	struct buffer_head *bh = NULL;

	char *buffer;
	size_t nbytes;

	if (*ppos >= zfs_inode->file_size)
		return 0;

	bh = sb_bread(filp->f_path.dentry->d_inode->i_sb, zfs_inode->dno);

	if (!bh) {
		dbg_printf("Fail to get block %lld when reading.\n", zfs_inode->dno);
		return 0;
	}

	buffer = (char *)bh->b_data;
	nbytes = min((size_t)zfs_inode->file_size, len);

	if (copy_to_user(buf, buffer, nbytes)) {
		brelse(bh);
		dbg_printf("Fail to read.\n");
		return -EFAULT;
	}

	brelse(bh);

	*ppos += nbytes;

	return nbytes;
}

static ssize_t zerofs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	struct super_block *sb = filp->f_path.dentry->d_inode->i_sb;
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct zerofs_super_block *zfs_sb;
	struct zerofs_inode *zfs_inode;
	struct buffer_head *bh = NULL;

	char *buffer;
	int ret;

	zfs_sb = sb->s_fs_info;
	zfs_inode = inode->i_private;

	bh = sb_bread(sb, zfs_inode->dno);
	if (!bh) {
		dbg_printf("Fail to write.\n");
		return 0;
	}

	buffer = (char *)bh->b_data;
	buffer += *ppos;

	if (copy_from_user(buffer, buf, len)) {
		brelse(bh);
		dbg_printf("Fail to write.\n");
		return -EFAULT;
	}

	*ppos += len;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if (mutex_lock_interruptible(&zerofs_inode_lock)) {
		dbg_printf("Fail to acquire lock.\n");
		return -EINTR;
	}

	zfs_inode->file_size = *ppos;
	ret = zerofs_sync_inode(sb, zfs_inode);
	if (ret) {
		len = ret;
	}
	
	mutex_unlock(&zerofs_inode_lock);

	return len;
}

static int zerofs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	return zerofs_create_dir_or_file(dir, dentry, S_IFDIR | mode);
}

static int zerofs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return zerofs_create_dir_or_file(dir, dentry, mode);
}

static struct dentry *zerofs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	struct zerofs_inode *zerofs_dir = dir->i_private;
	struct super_block *sb = dir->i_sb;

	struct buffer_head *bh = NULL;
	struct zerofs_dir_record *zerofs_record;

	int i;

	if (dentry->d_name.len > ZEROFS_NAME_LEN)
		return NULL;

	bh = sb_bread(sb, zerofs_dir->dno);
	BUG_ON(!bh);

	zerofs_record = (struct zerofs_dir_record *)bh->b_data;
	for (i = 0; i < zerofs_dir->children_count; i++) {
		if (!strcmp(zerofs_record->filename, dentry->d_name.name)) {
			struct inode *inode = zerofs_iget(sb, zerofs_record->ino);
			inode_init_owner(inode, dir, ((struct zerofs_inode *)inode->i_private)->mode);
			d_add(dentry, inode);
			return NULL;
		}
		zerofs_record++;
	}

	return NULL;
}

static int zerofs_create_dir_or_file(struct inode *dir, struct dentry *dentry, umode_t mode) 
{
	struct super_block *sb = dir->i_sb;
	struct zerofs_super_block *zfs_sb = sb->s_fs_info;
	struct inode *inode;
	struct zerofs_inode *zfs_inode;
	struct zerofs_inode *zfs_dir_inode;
	struct zerofs_dir_record *zfs_dir_reocrd;

	struct inode_operations *i_op;
	struct file_operations *i_fop;

	uint64_t count;
	int ret;

	if (mutex_lock_interruptible(&zerofs_dir_lock)) {
		dbg_printf("Fail to acquire lock when creating objects.\n");
		return -EINTR;
	}

	count = zfs_sb->inode_count;
	if (count >= ZEROFS_MAX_FS_OBJECTS) {
		dbg_printf("Maximum number of file system objects is reached.\n");
		mutex_unlock(&zerofs_dir_lock);
		return -EINVAL;
	}

	if (!S_ISDIR(mode) && !S_ISREG(mode)) {
		dbg_printf("Only supporting creating files or directories.\n");
		mutex_unlock(&zerofs_dir_lock);
		return -EINVAL;
	}

	inode = new_inode(sb);

	inode->i_sb = sb;
	i_op = (struct inode_operations *)kzalloc(sizeof(struct inode_operations), GFP_KERNEL);
	i_op->create = zerofs_create;
	i_op->lookup = zerofs_lookup;
	i_op->mkdir = zerofs_mkdir;
	inode->i_op = i_op;

	i_fop = (struct file_operations *)kzalloc(sizeof(struct file_operations), GFP_KERNEL);
	if (S_ISDIR(mode)) {
		i_fop->iterate = zerofs_iterate;
	} else if (S_ISREG(mode)) {
		i_fop->read = zerofs_read;
		i_fop->write = zerofs_write;
	}
	inode->i_fop = i_fop;

	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_ino = ZEROFS_START_INO + 1 + (count - ZEROFS_RESERVED_INODES);

	zfs_inode = kmem_cache_alloc(zerofs_inode_cachep, GFP_KERNEL);
	zfs_inode->ino = inode->i_ino;
	inode->i_private = zfs_inode;
	zfs_inode->mode = mode;

	if (S_ISDIR(mode))
		zfs_inode->children_count = 0;
	else if (S_ISREG(mode)) 
		zfs_inode->file_size = 0;

	if ((ret = zerofs_get_datablock(sb, &zfs_inode->dno)) < 0) {
		dbg_printf("Fail to get a free data block.\n");
		mutex_unlock(&zerofs_dir_lock);
		return ret;
	}

	zerofs_add_inode(sb, zfs_inode);

	zfs_dir_inode = dir->i_private;
	bh = sb_bread(sb, zfs_dir_inode->dno);
	BUG_ON(!bh);
	
	zfs_dir_record = (struct zerofs_dir_record *)bh->b_data;
	zfs_dir_record += zfs_dir_inode->children_count;
	zfs_dir_record->ino = zfs_inode->ino;
	strcpy(zfs_dir_record->filename, dentry->d_name.name);
	
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if (mutex_lock_interruptible(&zerofs_inode_lock)) {
		mutex_unlock(&zerofs_dir_lock);
		return -EINTR;
	}

	zfs_dir_inode->children_count++;
	ret = zerofs_sync_inode(sb, zfs_dir_inode);
	if (ret) {
		mutex_unlock(&zerofs_inode_lock);
		mutex_unlock(&zerofs_dir_lock);
		return ret;
	}

	mutext_unlock(&zerofs_inode_lock);
	mutext_unlock(&zerofs_dir_lock);

	inode_init_owner(inode, dir, mode);
	d_add(dentry, inode);	

	return 0;
}

static struct inode *zerofs_iget(struct super_block *sb, uint64_t ino) 
{
	struct inode *inode;
	struct zerofs_inode *zfs_inode;

	struct inode_operations *i_op;
	struct file_operations *i_fop;

	zfs_inode = zerofs_get_inode(sb, ino);

	inode = new_inode(sb);
	inode->i_ino = ino;
	inode->i_sb = sb;
	i_op = kzalloc(sizeof(struct inode_operations), GFP_KERNEL);
	i_op->create = zerofs_create;
	i_op->lookup = zerofs_lookup;
	i_op->mkdir = zerofs_mkdir;
	inode->i_op = i_op;

	i_fop = kmalloc(sizeof(struct file_operations), GFP_KERNEL);
	if (S_ISDIR(zfs_inode->mode)) {
		memset((void *)i_fop, 0, sizeof(struct file_operations));
		i_fop->iterate = zerofs_iterate;
	} else if (S_ISREG(zfs_inode->mode)) {
		memset((void *)i_fop, 0, sizeof(struct file_operations));
		i_fop->read = zerofs_read;
		i_fop->write = zerofs_write;
	}
	inode->i_fop = i_fop;

	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_private = zfs_inode;

	return inode;
}

static void zerofs_destroy_inode(struct inode *inode)
{
	struct zerofs_inode *zfs_inode = inode->i_private;
	kmem_cache_free(zerofs_inode_cachep, zfs_inode);
}

static int zerofs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh = NULL;
	struct inode *root_inode;	
	struct zerofs_super_block *zfs_sb;
	struct super_operations *s_op;
	struct inode_operations *i_op;
	struct file_operations *i_fop;	
	int ret;

	bh = sb_bread(sb, ZEROFS_SB_BLKNO);
	zfs_sb = (struct zerofs_super_block *)bh->b_data;

	if (zfs_sb->magic != ZEROFS_MAGIC) {
		ret = -EINVAL;
		goto fail;
	}

	if (zfs_sb->block_size != ZEROFS_DEFAULT_BLOCK_SIZE) {
		ret = -EINVAL;
		goto fail;
	}

	sb->s_magic = ZEROFS_MAGIC;
	sb->s_fs_info = zfs_sb;
	sb->s_maxbytes = ZEROFS_DEFAULT_BLOCK_SIZE;
	s_op = (struct super_operations *)kzalloc(sizeof(struct super_operations), GFP_KERNEL);
	s_op->destroy_inode = zerofs_destroy_inode;
	sb->s_op = s_op;

	root_inode = new_inode(sb);
	root_inode->i_ino = ZEROFS_ROOTDIR_INO;
	inode_init_owner(root_inode, NULL, S_IFDIR);
	root_inode->i_sb = sb;

	i_op = (struct inode_operations *)kzalloc(sizeof(struct inode_operations), GFP_KERNEL);
	i_op->create = zerofs_create;
	i_op->lookup = zerofs_lookup;
	i_op->mkdir = zerofs_mkdir;
	root_inode->i_op = i_op;

	i_fop = (struct file_operations *)kzalloc(sizeof(struct file_operations), GFP_KERNEL);
	i_fop->iterate = zerofs_iterate;
	root_inode->i_fop = i_fop;

	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = CURRENT_TIME;
	root_inode->i_private = (struct zerofs_inode *)zerofs_get_inode(sb, ZEROFS_ROOTDIR_INO);

	sb->s_root = d_make_root(root_inode);

	if (!sb->s_root) {
		ret = -ENOMEM;
		goto fail;
	}

fail:
	brelse(bh);

	return ret;
}

static struct dentry *zerofs_mount(struct file_system_type *fs_type,
				     int flags, const char *dev_name,
				     void *data)
{
	struct dentry *d;

	d = mount_bdev(fs_type, flags, dev_name, data, zerofs_fill_super);

	if (IS_ERR(d))
		dbg_printf("Mounting failed.\n");	
	else
		dbg_printf("Mounting successfully on %s\n", dev_name);

	return d;
}

static void zerofs_kill_superblock(struct super_block *sb)
{
	kill_block_super(sb);
}

struct file_system_type zerofs_type = {
	.owner = THIS_MODULE,
	.name = "zerofs",
	.mount = zerofs_mount,
	.kill_sb = zerofs_kill_superblock,
	.fs_flags = FS_REQUIRES_DEV,
};

static int zerofs_init(void)
{
	zerofs_inode_cachep = kmem_cache_create("zerofs_inode_cache", 
									sizeof(struct zerofs_inode),
									0,
									(SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
									NULL);
	
	if (!zerofs_inode_cache)
		return -ENOMEM;

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

