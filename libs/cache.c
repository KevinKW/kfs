/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/* Add list_head implementation */
#define container_of(ptr, type, member) ({                              \
                        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member)           \
        container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_add(struct list_head *new, struct list_head *prev,
		     struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void list_del(struct list_head *entry)
{
	struct list_head *prev = entry->prev;
	struct list_head *next = entry->next;

	next->prev = prev;
	prev->next = next;
}

static inline void list_del_init(struct list_head *entry)
{
    list_del(entry);
    INIT_LIST_HEAD(entry);
}

static inline void __list_splice(const struct list_head *list,
        struct list_head *prev,
        struct list_head *next)
{
    struct list_head *first = list->next;
    struct list_head *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}


static inline void list_splice(const struct list_head *list,
        struct list_head *head)
{
    if (!list_empty(list))
        __list_splice(list, head, head->next);
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
            &pos->member != (head);    \
            pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)          \
    for (pos = list_entry((head)->prev, typeof(*pos), member);  \
            &pos->member != (head);    \
            pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),  \
            n = list_entry(pos->member.next, typeof(*pos), member); \
            &pos->member != (head);                    \
            pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)      \
    for (pos = list_entry((head)->prev, typeof(*pos), member),  \
            n = list_entry(pos->member.prev, typeof(*pos), member); \
            &pos->member != (head);                    \
            pos = n, n = list_entry(n->member.prev, typeof(*n), member))


struct kcache_head {
    struct list_head *entries;
    pthread_mutex_t *locks;
    int kcache_size;
};

struct kcache_item {
    struct list_head link;
    void *data;
};

struct kcache_head *kcache_alloc(int kcache_size)
{
    struct kcache_head *kch;

    if (kcache_size <= 0) {
        return NULL;
    }

    kch = malloc(sizeof(*kch));
    if (!kch) {
        return NULL;
    }

    kch->entries = malloc(kcache_size * sizeof(*kch->entries));
    if (!kch->entries) {
        free(kch);
        return NULL;
    }

    kch->locks = malloc(kcache_size * sizeof(*kch->locks));
    if (!kch->locks) {
        free(kch->entries);
        free(kch);
        return NULL;
    }

    kch->kcache_size = kcache_size;
    while (kcache_size--) {
        INIT_LIST_HEAD(&kch->entries[kcache_size]);
        pthread_mutex_init(&kch->locks[kcache_size], NULL);
    }

    return kch;
}

void kcache_destroy(struct kcache_head *kch)
{
    free(kch->entries);
    free(kch->locks);
    free(kch);
}

void *kcache_find(struct kcache_head *kch,
        int (*test)(void *cachedata, void *data),
        void *data)
{
}

void *kcache_add(struct kchache_head *kch,
        int (*test)(void *cachedata, void *data),
        void *data)
{
}

void kcache_del(struct kcache_head *kch,
        int (*test)(void *cachedata, void *data),
        void *data)
{
}
