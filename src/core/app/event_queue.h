#ifndef UTIL_EVENT_QUEUE_H
#define UTIL_EVENT_QUEUE_H

#include "core/util/array.h"
#include "core/util/table.h"

typedef int (*observer_callback)(void *ctx, void *data);

struct observer {
    observer_callback callback;
    void *ctx;
    bool enabled;
};

struct observer_subject {
    struct array observers;
    bool is_currently_notifying;
    bool should_notify;
};

struct event {
    struct observer_subject *subject;
    void *data;
};

struct event_queue {
    struct table subjects;
    struct array pending;
};

int event_queue_init(struct event_queue *out);
void event_queue_destroy(struct event_queue *in);
int event_queue_add_subject(struct event_queue *in, const char *subject_id);
int event_queue_subject_add_observer(struct event_queue *in,
                                     const char *subject_id,
                                     observer_callback callback, void *ctx);
int event_queue_push(struct event_queue *in, const char *subject_id,
                     void *data);
int event_queue_update(struct event_queue *in);

int observer_subject_init(struct observer_subject *out);
void observer_subject_destroy(struct observer_subject *in);
int observer_subject_add_observer(struct observer_subject *in,
                                  observer_callback callback, void *ctx);
int observer_subject_notify(struct observer_subject *in, void *data);

#endif // UTIL_EVENT_QUEUE_H
