#include "core/app/event_queue.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include <stdio.h>

#define EVENT_QUEUE_START_CAPACITY 32
#define OBSERVER_START_CAPACITY 8

int event_queue_init(struct event_queue *out) {
    if (!out)
        return CORE_NULLPTR;

    int error = table_init(&out->subjects, EVENT_QUEUE_START_CAPACITY,
                           GLOBALS_STR_ID_MAX, sizeof(struct observer_subject));
    if (error)
        return error;

    error = array_init(&out->pending, EVENT_QUEUE_START_CAPACITY,
                       sizeof(struct event));
    if (error)
        return error;

    return CORE_SUCCESS;
}

void event_queue_destroy(struct event_queue *in) {
    if (!in)
        return;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->subjects);
    if (error)
        return;

    while (table_iterator_next(&iter)) {
        struct observer_subject *subject = iter.value;
        observer_subject_destroy(subject);
    }

    array_destroy(&in->pending);
    table_destroy(&in->subjects);
}

int event_queue_add_subject(struct event_queue *in, const char *subject_id) {
    if (!in || !subject_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", subject_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct observer_subject subject = {0};
    observer_subject_init(&subject);

    int error = table_insert(&in->subjects, id, &subject);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int event_queue_subject_add_observer(struct event_queue *in,
                                     const char *subject_id,
                                     observer_callback callback, void *ctx) {
    if (!in || !callback || !subject_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", subject_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct observer_subject *subject = NULL;
    int error = table_find((void **)&subject, &in->subjects, id);
    if (error)
        return error;

    error = observer_subject_add_observer(subject, callback, ctx);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int event_queue_push(struct event_queue *in, const char *subject_id,
                     void *data) {
    if (!in || !subject_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", subject_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct event new_event = {.data = data};

    int error = table_find((void **)&new_event.subject, &in->subjects, id);
    if (error)
        return error;

    error = array_push(&in->pending, &new_event);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int event_queue_update(struct event_queue *in) {
    if (!in)
        return CORE_NULLPTR;

    struct event *event = NULL;
    for (size_t i = 0; i < in->pending.length; ++i) {
        int error = array_get((void **)&event, &in->pending, i);
        if (error)
            continue;

        error = observer_subject_notify(event->subject, event->data);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Notifying observer subject failed");
            continue;
        }
    }

    array_clear(&in->pending);
    return CORE_SUCCESS;
}

int observer_subject_init(struct observer_subject *out) {
    if (!out)
        return CORE_NULLPTR;

    int error = array_init(&out->observers, OBSERVER_START_CAPACITY,
                           sizeof(struct observer));
    if (error)
        return error;

    out->is_currently_notifying = false;
    out->should_notify = true;
    return CORE_SUCCESS;
}

void observer_subject_destroy(struct observer_subject *subject) {
    if (!subject)
        return;

    array_destroy(&subject->observers);
}

int observer_subject_add_observer(struct observer_subject *in,
                                  observer_callback callback, void *ctx) {
    if (!in || !callback)
        return CORE_NULLPTR;

    struct observer new_observer = {.callback = callback, .enabled = true};
    if (ctx)
        new_observer.ctx = ctx;

    int error = array_push(&in->observers, &new_observer);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int observer_subject_notify(struct observer_subject *in, void *data) {
    if (!in)
        return CORE_NULLPTR;

    struct observer *current_observer = NULL;
    for (size_t i = 0; i < in->observers.length; ++i) {
        int error = array_get((void **)&current_observer, &in->observers, i);
        if (error)
            return error;

        if (current_observer && current_observer->callback)
            current_observer->callback(current_observer->ctx, data);
    }

    return CORE_SUCCESS;
}
