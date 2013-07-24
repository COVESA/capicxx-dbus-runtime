/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DBusClientId.h"
#include <typeinfo>

namespace CommonAPI {
namespace DBus {

DBusClientId::DBusClientId(std::string dbusId) :
                dbusId_(dbusId) {
}

bool DBusClientId::operator==(CommonAPI::ClientId& clientIdToCompare) {
    try {
        DBusClientId clientIdToCompareDBus = DBusClientId(dynamic_cast<DBusClientId&>(clientIdToCompare));
        return (&clientIdToCompareDBus == this);
    }
    catch (const std::bad_cast& e) {
        return false;
    }
}

bool DBusClientId::operator==(DBusClientId& clientIdToCompare) {
    return (clientIdToCompare.dbusId_ == dbusId_);
}

} /* namespace DBus */
} /* namespace CommonAPI */
