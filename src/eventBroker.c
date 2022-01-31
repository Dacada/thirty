#include <thirty/eventBroker.h>
#include <thirty/util.h>

#define EVENTS_INITIAL_CAPACITY 4
#define PENDING_EVENTS_INITIAL_CAPACITY 64

static struct growingArray events[EVENT_BROKER_EVENTS_TOTAL];
static struct growingArray pendingEvents;

struct eventCell {
        eventBrokerCallback callback;
        void *args;
        enum eventBrokerPriority prio;
};

struct postponedEventCell {
        eventBrokerCallback callback;
        void *reg_args;
        void *fire_args;
};

void eventBroker_startup(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_init(&events[i], sizeof(struct eventCell),
                                  EVENTS_INITIAL_CAPACITY);
        }
        growingArray_init(&pendingEvents, sizeof(struct postponedEventCell),
                          PENDING_EVENTS_INITIAL_CAPACITY);
}

void eventBroker_register(eventBrokerCallback cb, enum eventBrokerPriority prio,
                          enum eventBrokerEvent event, void *const args) {
        struct eventCell *e = growingArray_append(&events[event]);
        e->callback = cb;
        e->args = args;
        e->prio = prio;
}

void eventBroker_fire(enum eventBrokerEvent event, void *args) {
        growingArray_foreach_START(&events[event], struct eventCell *, e) {
                if (e->prio == EVENT_BROKER_PRIORITY_HIGH) {
                        e->callback(e->args, args);
                } else if (e->prio == EVENT_BROKER_PRIORITY_LOW) {
                        struct postponedEventCell *ee = growingArray_append(&pendingEvents);
                        ee->callback = e->callback;
                        ee->reg_args = e->args;
                        ee->fire_args = args;
                } else {
                        bail("invalid event broker priority");
                }
        } growingArray_foreach_END;
}

void eventBroker_runAsyncEvents(void) {
        growingArray_foreach_START(&pendingEvents, struct postponedEventCell *, e) {
                e->callback(e->reg_args, e->fire_args);
        } growingArray_foreach_END;
        growingArray_clear(&pendingEvents);
}

void eventBroker_shutdown(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_destroy(&events[i]);
        }
        growingArray_destroy(&pendingEvents);
}
