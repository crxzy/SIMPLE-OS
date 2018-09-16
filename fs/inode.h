#ifndef __INODE_H__
#define __INODE_H__
#include "device/ide.h"
#include "lib/kernel/list.h"
#include "lib/stdint.h"

// inode结构
struct inode {
    uint32_t i_no; // inode编号

    //大小
    uint32_t i_size;

    uint32_t i_open_cnts; // 记录此文件被打开的次数
    bool write_deny; // 写文件不能并行,进程写文件前检查此标识

    // i_sectors[0-11]是直接块, i_sectors[12]一级间接块指针 
    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};

struct inode *inode_open(struct partition *part, uint32_t inode_no);
void inode_sync(struct partition *part, struct inode *inode, void *io_buf);
void inode_init(uint32_t inode_no, struct inode *new_inode);
void inode_close(struct inode *inode);
void inode_release(struct partition *part, uint32_t inode_no);
void inode_delete(struct partition *part, uint32_t inode_no, void *io_buf);
#endif
