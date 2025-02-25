#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

static inline int cmp_val(const char *a, const char *b, bool descend)
{
    return descend ? -strcmp(a, b) : strcmp(a, b);
}

static inline int q_cmp(bool descend,
                        const struct list_head *a,
                        const struct list_head *b)
{
    const element_t *ea = list_entry(a, element_t, list);
    const element_t *eb = list_entry(b, element_t, list);
    int cmp_res = strcmp(ea->value, eb->value);
    return descend ? -cmp_res : cmp_res;
}

element_t *create_entry(const char *s)
{
    element_t *entry = malloc(sizeof(element_t));
    if (!entry)
        return NULL;

    char *s_dup = strdup(s);
    if (!s_dup) {
        free(entry);
        return NULL;
    }

    entry->value = s_dup;

    return entry;
}

struct list_head *q_find_mid(struct list_head *head)
{
    // The middle node of a linked list of size n is the
    // ⌊n / 2⌋ th node from the start using 0-based indexing.

    if (!head || list_empty(head))
        return NULL;

    // handle the 1-node case
    if (list_is_singular(head))
        return head->next;

    // find the middle node
    struct list_head *slow = head->next, *fast = slow->next->next;
    while (fast != head && fast->next != head) {
        slow = slow->next;
        fast = fast->next->next;
    }

    return slow->next;
}

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;
    INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    element_t *entry = NULL, *safe = NULL;
    list_for_each_entry_safe (entry, safe, head, list) {
        q_release_element(entry);
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *entry = create_entry(s);
    if (!entry)
        return false;
    list_add(&entry->list, head);

    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *entry = create_entry(s);
    if (!entry)
        return false;
    list_add_tail(&entry->list, head);

    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *entry = list_first_entry(head, element_t, list);

    if (sp && bufsize > 0) {
        strncpy(sp, entry->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del_init(&entry->list);
    return entry;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *entry = list_last_entry(head, element_t, list);

    if (sp && bufsize > 0) {
        strncpy(sp, entry->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del_init(&entry->list);
    return entry;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;
    int len = 0;
    struct list_head *li;
    list_for_each (li, head)
        len++;

    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/

    if (!head || list_empty(head))
        return false;

    // find the middle node
    struct list_head *mid = q_find_mid(head);

    element_t *target = list_entry(mid, element_t, list);
    // delete node from the list
    list_del(&target->list);
    q_release_element(target);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/

    if (!head || list_empty(head))
        return false;

    struct list_head *curr, *next;

    list_for_each_safe (curr, next, head) {
        element_t *entry = list_entry(curr, element_t, list);
        if (next != head &&
            strcmp(entry->value, list_entry(next, element_t, list)->value) ==
                0) {
            while (next != head &&
                   strcmp(entry->value,
                          list_entry(next, element_t, list)->value) == 0) {
                list_del(next);
                q_release_element(list_entry(next, element_t, list));
                next = curr->next;
            }
            list_del(curr);
            q_release_element(entry);
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    struct list_head *curr, *next;
    list_for_each_safe (curr, next, head) {
        if (next == head)
            break;
        list_del(next);
        list_add(next, curr->prev);
        next = curr->next;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *curr, *next;
    list_for_each_safe (curr, next, head) {
        list_del(curr);
        list_add(curr, head);
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head || list_empty(head) || k <= 1)
        return;
    // nodes waiting to be add & start add point
    struct list_head *pending = head->next, *last = head;
    struct list_head *curr, *next, *next_pend;
    int count = 0;
    list_for_each_safe (curr, next, head) {
        if (++count >= k) {
            while (count) {
                count--;
                next_pend = pending->next;
                list_del(pending);
                list_add(pending, last);
                pending = next_pend;
            }
            last = next->prev;
        }
    }
}

static struct list_head *merge_two_sorted_list(bool descend,
                                               struct list_head *a,
                                               struct list_head *b)
{
    struct list_head guard;
    struct list_head *tail = &guard;

    while (a && b) {
        if (q_cmp(descend, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
        }
    }

    if (a) {
        tail->next = a;
        a->prev = tail;
    } else if (b) {
        tail->next = b;
        b->prev = tail;
    } else {
        tail->next = NULL;
    }
    return guard.next;
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/

    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return 1;

    struct list_head *tail = head->next, *curr, *tmp;

    list_for_each (curr, head) {
        while (tail != head &&
               strcmp(list_entry(curr, element_t, list)->value,
                      list_entry(tail, element_t, list)->value) < 0) {
            tmp = tail->prev;
            list_del(tail);
            q_release_element(list_entry(tail, element_t, list));
            tail = tmp;
        }
        tail = curr;
    }

    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/

    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return 1;

    struct list_head *tail = head->next, *curr, *tmp;

    list_for_each (curr, head) {
        while (tail != head &&
               strcmp(list_entry(curr, element_t, list)->value,
                      list_entry(tail, element_t, list)->value) > 0) {
            tmp = tail->prev;
            list_del(tail);
            q_release_element(list_entry(tail, element_t, list));
            tail = tmp;
        }
        tail = curr;
    }

    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head || list_empty(head))
        return 0;

    queue_contex_t *curr = NULL, *ans_entry;

    struct list_head *result = NULL;

    list_for_each_entry (curr, head, chain) {
        curr->q->prev->next = NULL;
        result = merge_two_sorted_list(descend, result, curr->q->next);
        curr->q->next = curr->q;
    }

    ans_entry = list_entry(head->next, queue_contex_t, chain);
    INIT_LIST_HEAD(ans_entry->q);

    while (result) {
        struct list_head *next = result->next;
        list_add_tail(result, ans_entry->q);
        result = next;
    }

    return q_size(ans_entry->q);
}

void q_sort(struct list_head *head, bool descend)
{
    struct list_head *list;
    struct list_head *part[65];
    int level;

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    // break circular
    head->prev->next = NULL;
    head->prev = NULL;

    list = head->next;
    INIT_LIST_HEAD(head);

    // init part[]
    memset(part, 0, sizeof(part));

    while (list) {
        struct list_head *curr = list;
        struct list_head *next = list->next;

        curr->prev = NULL;
        curr->next = NULL;

        list = next;

        // the number of nodes in part[] will always be 0 or 2^level
        // [1, 2, 4, 8, 16...] or [0, 2, 0, 0, 16] are acceptable
        // depends on level (idx)
        // keep merging until unable to merge
        for (level = 0; part[level]; level++) {
            curr = merge_two_sorted_list(descend, part[level], curr);
            part[level] = NULL;
        }
        part[level] = curr;
    }

    list = NULL;
    for (level = 0; level < (int) (sizeof(part) / sizeof(part[0])); level++) {
        if (!part[level])
            continue;
        list = merge_two_sorted_list(descend, part[level], list);
    }

    // reconstruct list
    while (list) {
        struct list_head *next = list->next;
        list_add_tail(list, head);
        list = next;
    }
}