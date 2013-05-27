/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dbus/dbus.h>
#include <CommonAPI/DBus/DBusServiceRegistry.h>


inline char eliminateZeroes(char val) {
    return !val ? '0' : val;
}


inline void printLibdbusMessageBody(char* data, uint32_t fromByteIndex, uint32_t toByteIndex) {
	for(int i = fromByteIndex; i < toByteIndex; i++) {
		std::cout << eliminateZeroes(data[i]);
		if(i%8 == 7) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}

inline void printLibdbusMessage(DBusMessage* libdbusMessage, uint32_t fromByteIndex, uint32_t toByteIndex) {
    char* data = dbus_message_get_body(libdbusMessage);
    printLibdbusMessageBody(data, fromByteIndex, toByteIndex);
}

inline void printLibdbusMessage(DBusMessage* libdbusMessage) {
    printLibdbusMessage(libdbusMessage, 0, dbus_message_get_body_length(libdbusMessage));
}

inline std::string toString(CommonAPI::DBus::DBusServiceRegistry::DBusServiceState state) {
    switch(state) {
        case CommonAPI::DBus::DBusServiceRegistry::DBusServiceState::AVAILABLE:
            return "AVAILABLE";
        case CommonAPI::DBus::DBusServiceRegistry::DBusServiceState::NOT_AVAILABLE:
            return "NOT_AVAILABLE";
        case CommonAPI::DBus::DBusServiceRegistry::DBusServiceState::RESOLVED:
            return "RESOLVED";
        case CommonAPI::DBus::DBusServiceRegistry::DBusServiceState::RESOLVING:
            return "RESOLVING";
        case CommonAPI::DBus::DBusServiceRegistry::DBusServiceState::UNKNOWN:
            return "UNKNOWN";
    }
}

inline std::string toString(CommonAPI::AvailabilityStatus state) {
    switch(state) {
        case CommonAPI::AvailabilityStatus::AVAILABLE:
            return "AVAILABLE";
        case CommonAPI::AvailabilityStatus::NOT_AVAILABLE:
            return "NOT_AVAILABLE";
        case CommonAPI::AvailabilityStatus::UNKNOWN:
            return "UNKNOWN";
    }
}

inline std::string toString(CommonAPI::CallStatus state) {
    switch(state) {
        case CommonAPI::CallStatus::CONNECTION_FAILED:
            return "CONNECTION_FAILED";
        case CommonAPI::CallStatus::NOT_AVAILABLE:
            return "NOT_AVAILABLE";
        case CommonAPI::CallStatus::OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case CommonAPI::CallStatus::REMOTE_ERROR:
            return "REMOTE_ERROR";
        case CommonAPI::CallStatus::SUCCESS:
            return "SUCCESS";
    }
}
