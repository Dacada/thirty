#include <thirty/eventBroker.h>
#include <thirty/util.h>

#define EVENTS_INITIAL_CAPACITY 4

static struct growingArray events[EVENT_BROKER_EVENTS_TOTAL];

struct eventCell {
        eventBrokerCallback callback;
        void *args;
};

void eventBroker_startup(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_init(&events[i], sizeof(struct eventCell),
                                  EVENTS_INITIAL_CAPACITY);
        }
}

void eventBroker_register(eventBrokerCallback cb, enum eventBrokerPriority prio,
                          enum eventBrokerEvent event, void *const args) {
        (void)prio;
        struct eventCell *e = growingArray_append(&events[event]);
        e->callback = cb;
        e->args = args;
}

void eventBroker_fire(enum eventBrokerEvent event, void *args) {
        growingArray_foreach_START(&events[event], struct eventCell *, e)
                e->callback(e->args, args);
        growingArray_foreach_END;
}

void eventBroker_runAsyncEvents(void) {
}

void eventBroker_shutdown(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_destroy(&events[i]);
        }
}
