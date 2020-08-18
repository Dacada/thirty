#include <eventBroker.h>
#include <dsutils.h>
#include <util.h>
#include <stdlib.h>
#include <string.h>

#define EVENTS_INITIAL_CAPACITY 4

static struct growingArray events[EVENT_BROKER_EVENTS_TOTAL];

void eventBroker_startup(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_init(&events[i], sizeof(eventBrokerCallback),
                                  EVENTS_INITIAL_CAPACITY);
        }
}

void eventBroker_register(eventBrokerCallback cb, enum eventBrokerPriority prio,
                          enum eventBrokerEvent event) {
        (void)prio;
        eventBrokerCallback *e = growingArray_append(&events[event]);
        *e = cb;
}

void eventBroker_fire(enum eventBrokerEvent event, void *args) {
        growingArray_foreach_START(&events[event], eventBrokerCallback*, cb)
                (*cb)(args);
        growingArray_foreach_END;
}

void eventBroker_runAsyncEvents(void) {
}

void eventBroker_shutdown(void) {
        for (size_t i=0; i<EVENT_BROKER_EVENTS_TOTAL; i++) {
                growingArray_destroy(&events[i]);
        }
}
