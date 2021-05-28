/*
 * (C) Copyright 2018, Davide Bettio, davide@uninstall.it.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#ifndef _ASTARTE_LIST_H_
#define _ASTARTE_LIST_H_

#include <stdbool.h>
#include <stdlib.h>

struct astarte_list_head_t
{
    struct astarte_list_head_t *next;
    struct astarte_list_head_t *prev;
};

struct astarte_ptr_list_entry_t
{
    struct astarte_list_head_t head;
    const void *value;
};

/**
 * @brief gets a pointer to the struct that contains a certain list head
 *
 * @details This macro should be used to retrieve a pointer to the struct that is containing the
 * given ListHead.
 */
#define GET_LIST_ENTRY(list_item, type, list_head_member)                                          \
    ((type *) (((char *) (list_item)) - ((unsigned long) &((type *) 0)->list_head_member)))

/**
 * @brief iterates through the list
 *
 * @details This macro should be used to iterate through the linked list without modifying it
 */
#define LIST_FOR_EACH(item, head) for (item = (head)->next; item != (head); item = item->next)

/**
 * @brief iterates through the list, possibly modifying it
 *
 * @details This macro should be used to iterate through the linked list when there is the need
 * to modify it while iterating
 */
#define MUTABLE_LIST_FOR_EACH(item, tmp, head)                                                     \
    for (item = (head)->next, tmp = item->next; item != (head); item = tmp, tmp = item->next)

/**
 * @brief Inserts a linked list head between two linked list heads
 *
 * @details It inserts a linked list head between prev_head and next_head.
 * @param new_item the linked list head that will be inserted to the linked list
 * @param prev_head the linked list head that comes before the element that is going to be inserted
 * @param next_head the linked list head that comes after the element that is going to be inserted
 */
static inline void astarte_list_insert(struct astarte_list_head_t *new_item,
    struct astarte_list_head_t *prev_head, struct astarte_list_head_t *next_head)
{
    new_item->prev = prev_head;
    new_item->next = next_head;
    next_head->prev = new_item;
    prev_head->next = new_item;
}

/**
 * @brief Appends a list item to a linked list
 *
 * @details It appends a list item head to a linked list and it initializes linked list pointer if
 * empty.
 * @param list a pointer to the linked list pointer that the head is going to be append, it will be
 * set to new_item if it is the first one
 * @param new_item the item that is going to be appended to the end of the list
 */
static inline void astarte_list_append(
    struct astarte_list_head_t *head, struct astarte_list_head_t *new_item)
{
    astarte_list_insert(new_item, head->prev, head);
}

/**
 * @brief Prepends a list item to a linked list
 *
 * @details It prepends a list item head to a linked list and it updates the pointer to the list.
 * @param list a pointer to the linked list
 * @param new_item the list head that is going to be prepended to the list
 */
static inline void astarte_list_prepend(
    struct astarte_list_head_t *head, struct astarte_list_head_t *new_item)
{
    astarte_list_insert(new_item, head, head->next);
}

/**
 * @brief Removes a linked list item from a linked list
 *
 * @details It removes a linked list head from the list pointed by list.
 * @param list a pointer to the linked list pointer that we want to remove the item from, it will be
 * set to NULL if no items are left
 * @param remove_item the item that is going to be removed
 */
static inline void astarte_list_remove(struct astarte_list_head_t *remove_item)
{
    remove_item->prev->next = remove_item->next;
    remove_item->next->prev = remove_item->prev;
}

/**
 * @brief Initializes a linked list
 *
 * @details This function must be called to initialized a newly created list
 * @param head a pointer to the linked list
 */
static inline void astarte_list_init(struct astarte_list_head_t *head)
{
    head->prev = head;
    head->next = head;
}

/**
 * @brief Checks if the linked list is empty
 *
 * @details This function returns true if the linked list is empty
 * @return true if the list is empty, false otherwise
 */
static inline bool astarte_list_is_empty(struct astarte_list_head_t *list_item)
{
    return (list_item->next == list_item) && (list_item->prev == list_item);
}

/**
 * @brief Returns the first item of the list
 *
 * @details This function returns the first item of the linked list pointed by head
 * @param head a pointer to the linked list
 * @return A pointer to the first item of the list
 */
static inline struct astarte_list_head_t *astarte_list_first(struct astarte_list_head_t *head)
{
    return head->next;
}

/**
 * @brief Returns the last item of the list
 *
 * @details This function returns the last item of the linked list pointed by head
 * @param head a pointer to the linked list
 * @return A pointer to the last item of the list
 */
static inline struct astarte_list_head_t *astarte_list_last(struct astarte_list_head_t *head)
{
    return head->prev;
}

#endif
