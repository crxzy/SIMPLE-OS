#include "list.h"
#include "interrupt.h"

/* 初始化双向链表list */
void list_init(struct list *list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

/* 把链表元素elem插入在元素before之前 */
void list_insert_before(struct list_elem *before, struct list_elem *elem) {
    enum intr_status old_status = intr_disable();

    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;

    intr_set_status(old_status);
}

void list_push(struct list *plist, struct list_elem *elem) {
    list_insert_before(plist->head.next, elem); // 在队头插入elem
}

void list_append(struct list *plist, struct list_elem *elem) {
    list_insert_before(&plist->tail, elem); // 在队尾的前面插入
}

void list_remove(struct list_elem *pelem) {
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status);
}

struct list_elem *list_pop(struct list *plist) {
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}

/* 从链表中查找元素obj_elem,成功时返回true,失败时返回false */
bool elem_find(struct list *plist, struct list_elem *obj_elem) {
    struct list_elem *elem = plist->head.next;
    while (elem != &plist->tail) {
        if (elem == obj_elem) {
            return true;
        }
        elem = elem->next;
    }
    return false;
}


struct list_elem *list_traversal(struct list *plist, function func, int arg) {
    struct list_elem *elem = plist->head.next;
    /* 如果队列为空,就必然没有符合条件的结点,故直接返回NULL */
    if (list_empty(plist)) {
        return NULL;
    }

    while (elem != &plist->tail) {
        if (func(elem, arg)) {
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

/* 返回链表长度 */
uint32_t list_len(struct list *plist) {
    struct list_elem *elem = plist->head.next;
    uint32_t length = 0;
    while (elem != &plist->tail) {
        length++;
        elem = elem->next;
    }
    return length;
}

bool list_empty(struct list *plist) { // 判断队列是否为空
    return (plist->head.next == &plist->tail ? true : false);
}
