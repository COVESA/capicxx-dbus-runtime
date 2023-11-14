
#include <dbus/dbus.h>

int main(void) {
    auto z = dbus_connection_send_with_reply_set_notify(nullptr, nullptr, nullptr,
                                                 nullptr, nullptr, nullptr, 0);
    return 0;
}
