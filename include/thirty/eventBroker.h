#ifndef EVENT_BROKER_H
#define EVENT_BROKER_H

/*
 * A system to handle firing of events and callbacks in response. It acts
 * globally, it's not an "object". Anything can use it to register a callback
 * for an event type, and anything can use it to fire an event type, which
 * results in all its callbacks being executed. Callbacks are regstered with a
 * priority, which indicates when exactly will they be executed.
 */

/*
 * A callback must accept two void pointers. The first pointer are parameters
 * set up upon event registration. The second pointer contains the parameters
 * of the event given when fired. The first one can be anything, but the second
 * one should correspond with the eventBrokerArgs* struct of that event, or be
 * NULL.
 */
typedef void(*eventBrokerCallback)(void*, void*);

/*
 * Event callbacks must be registered with a priority. High priority means they
 * will run synchronously the moment the event fires. Low priority means they
 * will run from the main thread at the end of the current frame.
 */
enum eventBrokerPriority {
        EVENT_BROKER_PRIORITY_HIGH,
        EVENT_BROKER_PRIORITY_LOW,
};

/*
 * The event a task is associated with. Parameters given on firing the event
 * should be the struct that coincides with the event enum name. Some of these
 * events are for the game loop or window system and it makes little sense to
 * fire them. They're just for registering to them.
 */
enum eventBrokerEvent {
        // Only fires once before the main game loop starts
        EVENT_BROKER_SETUP,
        
        // Draw step
        EVENT_BROKER_DRAW,

        // Update step (should always use high priority events)
        EVENT_BROKER_UPDATE,

        // Only fires once just before the system shuts down
        EVENT_BROKER_TEAR_DOWN,

        // Window has been resized
        EVENT_BROKER_WINDOW_RESIZED,
        
        // A keyboard key has been pressed, released, etc... Useful for
        // reacting to a single keypress event: "Press E to activate"
        EVENT_BROKER_KEYBOARD_EVENT,

        // Poll keyboard key status. Useful for keys that are pressed for
        // several frames: "Use W to walk forward"
        EVENT_BROKER_KEYBOARD_POLL,

        // Mouse position has changed
        EVENT_BROKER_MOUSE_POSITION,

        // Mouse has scrolled
        EVENT_BROKER_MOUSE_SCROLL,

        // Mouse button has been pressed or released
        EVENT_BROKER_MOUSE_BUTTON,

        // Poll mouse button status. Useful for keys that are pressed for
        // several frames: "Use M1+M2 to walk forward"
        EVENT_BROKER_MOUSE_POLL,

        // An amount of TCP data is ready to be received from the network
        // (should always be high priority, the event will keep triggering
        // until the socket is read)
        EVENT_BROKER_TCP_RECV,
        
        // A UDP datagram is ready to be received from the network (should
        // always be high priority, the event will keep triggering until the
        // socket is read)
        EVENT_BROKER_UDP_RECV,
        
        // It is possible to send TCP data through the network (should always
        // be high priority, the event will keep triggering until the socket is
        // written to)
        EVENT_BROKER_TCP_SEND,
        
        // It is possible tp send a UDP datagram through the network (should
        // always be high priority, the event will keep triggering until the
        // socket is written to)
        EVENT_BROKER_UDP_SEND,

        // Not an event, just find out how many events there are.
        EVENT_BROKER_EVENTS_TOTAL
};

/*
 * These are the structs of the arguments of each event type. Empty ones simply
 * don't have arguments and should be NULL.
 */

struct eventBrokerSetup {
};
struct eventBrokerDraw {
};

struct eventBrokerUpdate {
        const float timeDelta;
};

struct eventBrokerTearDown {
};

struct eventBrokerWindowResized {
        const int width;
        const int height;
};

// Check GLFW documentation for more information about the meaning of each
// field.
struct eventBrokerKeyboardEvent {
        const int key;
        const int action;
        const int modifiers;
};

// Use game_keyPressed to check keys
struct eventBrokerKeyboardInput {
};

struct eventBrokerMousePosition {
        const double xpos;
        const double ypos;
};

struct eventBrokerMouseScroll {
        const double amount;
};

struct eventBrokerMouseButton {
        const int button;
        const int action;
        const int modifiers;
};

// Use game_keyPressed to check buttons
struct eventBrokerMousePoll {
};

struct eventBrokerTCPRecv {
        int socket;
};

struct eventBrokerUDPRecv {
        int socket;
};

struct eventBrokerTCPSend {
        int socket;
};

struct eventBrokerUDPSend {
        int socket;
};

/*
 * Start up the message broker system.
 */
void eventBroker_startup(void);

/*
 * Register a callback to run when an event is fired with the given
 * priority. The event will be called with the argument supplied here and with
 * the arguments supplied by the one firing it.
 */
void eventBroker_register(eventBrokerCallback cb,
                          enum eventBrokerPriority prio,
                          enum eventBrokerEvent event, void *args)
        __attribute__((nonnull (1)));

/*
 * Fire an event, immediately calling all its high priority callbacks and
 * setting up the others to be called eventually. Args should be NULL or a
 * struct of the type corresponding to the event, so the callbacks know what to
 * expect.
 */
void eventBroker_fire(enum eventBrokerEvent event, void *args);

/*
 * Run all low priority events, called at the end of a frame.
 */
void eventBroker_runAsyncEvents(void);

/*
 * Call all pending events, then cleanup and free resources.
 */
void eventBroker_shutdown(void);

#endif
