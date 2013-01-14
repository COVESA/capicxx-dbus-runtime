/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusOutputStream.h"

namespace CommonAPI {
namespace DBus {

DBusOutputStream::DBusOutputStream(DBusMessage dbusMessage) :
                dbusMessage_(dbusMessage) {
}

DBusOutputStream::~DBusOutputStream() {
}

void DBusOutputStream::flush() {
    const int toWrite = payload_.size();
    const bool success = dbusMessage_.setBodyLength(toWrite);
    char* destinationDataPtr = dbusMessage_.getBodyData();

    memcpy(destinationDataPtr, payload_.c_str(), toWrite);
}

void DBusOutputStream::reserveMemory(size_t numOfBytes) {
    assert(numOfBytes >= 0);

    payload_.reserve(numOfBytes);
}

DBusOutputStream& DBusOutputStream::writeString(const char* cString, const uint32_t& length) {
    assert(cString != NULL);
    assert(cString[length] == '\0');

    *this << length;

    writeRawData(cString, length + 1);

    return *this;
}

} // namespace DBus
} // namespace CommonAPI
