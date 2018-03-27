#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xce68c3a5, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xd8f7d622, __VMLINUX_SYMBOL_STR(kmem_cache_destroy) },
	{ 0xb21df0ac, __VMLINUX_SYMBOL_STR(unregister_filesystem) },
	{ 0xbdde578e, __VMLINUX_SYMBOL_STR(register_filesystem) },
	{ 0xebe0924e, __VMLINUX_SYMBOL_STR(kmem_cache_create) },
	{ 0x362ef408, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0x4786a55b, __VMLINUX_SYMBOL_STR(kmem_cache_free) },
	{ 0xe92356ab, __VMLINUX_SYMBOL_STR(d_add) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0xe663c032, __VMLINUX_SYMBOL_STR(d_make_root) },
	{ 0xd2b1e34, __VMLINUX_SYMBOL_STR(current_time) },
	{ 0xd4a814fa, __VMLINUX_SYMBOL_STR(inode_init_owner) },
	{ 0x3aa95977, __VMLINUX_SYMBOL_STR(new_inode) },
	{ 0xd60c0552, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x510f7142, __VMLINUX_SYMBOL_STR(mount_bdev) },
	{ 0x7e0a0659, __VMLINUX_SYMBOL_STR(kill_block_super) },
	{ 0xbf63e976, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xb07a96dc, __VMLINUX_SYMBOL_STR(sync_dirty_buffer) },
	{ 0x57c4533f, __VMLINUX_SYMBOL_STR(mark_buffer_dirty) },
	{ 0x269cfd2a, __VMLINUX_SYMBOL_STR(mutex_lock_interruptible) },
	{ 0xaa6ab953, __VMLINUX_SYMBOL_STR(__brelse) },
	{ 0xb44ad4b3, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x88db9f48, __VMLINUX_SYMBOL_STR(__check_object_size) },
	{ 0xd68c59ab, __VMLINUX_SYMBOL_STR(__bread_gfp) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "13B5E5BDEC8D06C92B00B4D");
