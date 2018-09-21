#include "inode.h"
#include "debug.h"
#include "file.h"
#include "fs.h"
#include "global.h"
#include "interrupt.h"
#include "list.h"
#include "stdio-kernel.h"
#include "string.h"
#include "memory.h"
#include "super_block.h"

// 用来存储inode位置
struct inode_position {
    bool two_sec;      // inode是否跨扇区
    uint32_t sec_lba;  // inode所在的扇区号
    uint32_t off_size; // inode在扇区内的字节偏移量
};

// 获取inode所在的扇区和扇区内的偏移量
static void inode_locate(struct partition *part, uint32_t inode_no,
                         struct inode_position *inode_pos) {
    /* inode_table在硬盘上是连续的 */
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba;

    uint32_t inode_size = sizeof(struct inode);
    // 第N个inode的字节
    uint32_t off_size = inode_no * inode_size;
    // 所在的扇区
    uint32_t off_sec = off_size / 512;
    // 扇区内的字节偏移量
    uint32_t off_size_in_sec = off_size % 512;

    // 判断此i结点是否跨越2个扇区
    uint32_t left_in_sec = 512 - off_size_in_sec;
    if (left_in_sec < inode_size) {
        inode_pos->two_sec = true;
    } else {
        inode_pos->two_sec = false;
    }
    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

// 将inode写入到分区part
void inode_sync(struct partition *part, struct inode *inode, void *io_buf) {
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));

    // i_open_cnts write_deny inode_tag 只在内存存在
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false; // 置为false,以保证在硬盘中读出时为可写
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

    char *inode_buf = (char *)io_buf;
    if (inode_pos.two_sec) { // 若是跨了两个扇区,就要读出两个扇区再写入两个扇区
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode,
               sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode,
               sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

/* 根据i结点号返回相应的i结点 */
struct inode *inode_open(struct partition *part, uint32_t inode_no) {
    // 先在已打开inode链表中找inode
    struct list_elem *elem = part->open_inodes.head.next;
    struct inode *inode_found;
    while (elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if (inode_found->i_no == inode_no) {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }

    // 在open_inodes链表中找不到

    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);

    /* 为使通过sys_malloc创建的新inode被所有任务共享,
     * 需要将inode置于内核空间,故需要临时
     * 将cur_pbc->pgdir置为NULL */
    struct task_struct *cur = running_thread();
    uint32_t *cur_pagedir_bak = cur->pgdir;
    cur->pgdir = NULL;
    // sys_malloc 将分配内核内存
    inode_found = (struct inode *)sys_malloc(sizeof(struct inode));
    /* 恢复pgdir */
    cur->pgdir = cur_pagedir_bak;

    char *inode_buf;
    if (inode_pos.two_sec) {
        inode_buf = (char *)sys_malloc(1024);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
        inode_buf = (char *)sys_malloc(512);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    // 加入已打开链表
    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;

    sys_free(inode_buf);
    return inode_found;
}

// 关闭inode或减少inode的打开数
void inode_close(struct inode *inode) {
    enum intr_status old_status = intr_disable();
    if (--inode->i_open_cnts == 0) {
        list_remove(&inode->inode_tag); // 将I结点从part->open_inodes中去掉

        // 确保free的是内核内存
        struct task_struct *cur = running_thread();
        uint32_t *cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL;
        sys_free(inode);
        cur->pgdir = cur_pagedir_bak;
    }
    intr_set_status(old_status);
}

// 将硬盘分区part上的inode清空
void inode_delete(struct partition *part, uint32_t inode_no, void *io_buf) {
    ASSERT(inode_no < 4096);
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    char *inode_buf = (char *)io_buf;
    if (inode_pos.two_sec) {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
        memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

// 回收inode的数据块和inode本身
void inode_release(struct partition *part, uint32_t inode_no) {
    struct inode *inode_to_del = inode_open(part, inode_no);
    ASSERT(inode_to_del->i_no == inode_no);

    // 回收inode占用的所有块
    uint8_t block_idx = 0, block_cnt = 12;
    uint32_t block_bitmap_idx;
    uint32_t all_blocks[140] = {0}; // 12个直接块+128个间接块

    // 前12个直接块存入all_blocks
    while (block_idx < 12) {
        all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];
        block_idx++;
    }

    //如果一级间接块表存在,将其128个间接块读到all_blocks[12~],
    //并释放一级间接块表所占的扇区
    if (inode_to_del->i_sectors[12] != 0) {
        ide_read(part->my_disk, inode_to_del->i_sectors[12], all_blocks + 12,                  1);
        block_cnt = 140;

        // 回收一级间接块表占用的扇区
        block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
    }

    block_idx = 0;
    while (block_idx < block_cnt) {
        if (all_blocks[block_idx] != 0) {
            block_bitmap_idx = 0;
            block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            ASSERT(block_bitmap_idx > 0);
            bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
        }
        block_idx++;
    }

    // 回收该inode所占用的inode
    bitmap_set(&part->inode_bitmap, inode_no, 0);
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    // 调试用、 无需清零
    void *io_buf = sys_malloc(1024);
    inode_delete(part, inode_no, io_buf);
    sys_free(io_buf);

    inode_close(inode_to_del);
}

// 初始化new_inode
void inode_init(uint32_t inode_no, struct inode *new_inode) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    // 初始化块索引数组i_sector
    uint8_t sec_idx = 0;
    while (sec_idx < 13) {
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}
