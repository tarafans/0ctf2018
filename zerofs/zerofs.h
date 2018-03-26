#define ZEROFS_MAGIC 0x4f52455a

#define ZEROFS_DEFAULT_BLOCK_SIZE 4096

#define ZEROFS_NAME_LEN 255

#define ZEROFS_START_INO 0x1337

/* for sb, inode store, root_dir's data */
#define ZEROFS_RESERVED_INODES 3 

#ifdef DEBUG
#define dbg_printf(...) printk("[zerofs]: " __VA_ARGS__)
#else
#define dbg_printf(...) ((void) 0)
#endif


#define ZEROFS_SB_BLKNO 0
#define ZEROFS_IMAP_BLKNO 1

#define ZEROFS_ROOTDIR_INO 1
#define ZEROFS_ROOTDIR_BLKNO 2

#define ZEROFS_LAST_RESERVED_BLKNO ZEROFS_ROOTDIR_BLKNO
#define ZEROFS_LAST_RESERVED_INO ZEROFS_ROOTDIR_INO

struct zerofs_dir_record {
	char filename[ZEROFS_NAME_LEN];
	uint64_t ino;
};

struct zerofs_inode {
	uint64_t ino;
	uint64_t dno;

	mode_t mode;

	union {
		uint64_t file_size;
		uint64_t children_count;
	};
};

#define ZEROFS_MAX_FS_OBJECTS 64

struct zerofs_super_block {
	uint64_t magic;
	uint64_t block_size;
	uint64_t inode_count;
	uint64_t free_blocks;
	char padding[4064];
};
