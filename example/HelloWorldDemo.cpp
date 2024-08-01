#include <event2/event.h>
#include <iostream>

int main() {
    struct event_base* base = event_base_new();
    if (!base) {
        std::cerr << "Could not initialize libevent!\n";
        return 1;
    }
    std::cout << "libevent version: " << event_get_version() << "\n";
    event_base_free(base);
    return 0;
}
