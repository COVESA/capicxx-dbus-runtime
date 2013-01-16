/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dbus/dbus.h>


inline char eliminateZeroes(char val) {
    return !val ? '0' : val;
}

inline void printLibdbusMessage(DBusMessage* libdbusMessage, uint32_t fromByteIndex, uint32_t toByteIndex) {
    char* data = dbus_message_get_body(libdbusMessage);
    for(int i = fromByteIndex; i < toByteIndex; i++) {
        std::cout << eliminateZeroes(data[i]);
        if(i%8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}
