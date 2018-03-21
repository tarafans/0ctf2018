#define ZERO_MAGIC 0x4f52455a

#define ZERO_DEFAULT_BLOCK_SIZE 4096

#define ZERO_FILENAME_MAXLEN 255

#define ZERO_START_INO 10

/* for sb, inode store, data block */
#define ZERO_RESERVED_INODES 3 

#ifdef DEBUG
#define dbg_printf(fmt, ...) { \
	printk("[zerofs]:" fmt, ##__VA_ARGS__); \
}
#else
#define dbg_printf(fmt, ...) ((void) 0)
#endif

#define ZERO_ROOTDIR_INO 1
#define ZERO SB_BLKNO 0
#define ZERO_IMAP_BLKNO 1
#define ZERO_ROOTDIR_DNO 4

#define ZERO_LAST_RESERVED_BLK ZERO_ROOTDIR_DNO
#define ZERO_LAST_RESERVED_INO ZERO_ROOTDIR_INO

struct zerofs_dir_record {
	char filename[ZERO_FILENAME_MAXLEN];
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

#define ZERO_MAX_FS_OBJECTS 64

struct zero_super_block {
	uint64_t magic;
	uint64_t block_size;
	uint64_t inodes_count;
	uint64_t free_blocks;
	char padding[4064];
};
