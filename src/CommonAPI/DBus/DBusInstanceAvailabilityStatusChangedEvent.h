/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_INSTANCE_AVAILABILITY_STATUS_CHANGED_EVENT_H_
#define COMMONAPI_DBUS_DBUS_INSTANCE_AVAILABILITY_STATUS_CHANGED_EVENT_H_

#include <CommonAPI/ProxyManager.h>

#include "DBusProxy.h"
#include "DBusObjectManagerStub.h"
#include "DBusInstanceAvailabilityStatusChangedEvent.h"

#include <functional>
#include <future>
#include <string>
#include <vector>


namespace CommonAPI {
namespace DBus {

// TODO move logic to DBusServiceRegistry, now every proxy will deserialize the messages!
class DBusInstanceAvailabilityStatusChangedEvent:
                public ProxyManager::InstanceAvailabilityStatusChangedEvent,
                public DBusProxyConnection::DBusSignalHandler {
 public:
    DBusInstanceAvailabilityStatusChangedEvent(DBusProxy& dbusProxy, const std::string& interfaceName) :
                    dbusProxy_(dbusProxy),
                    observedInterfaceName_(interfaceName) {
    }

    virtual SubscriptionStatus onSignalDBusMessage(const DBusMessage& dbusMessage) {
        if (dbusMessage.hasMemberName("InterfacesAdded")) {
            onInterfacesAddedSignal(dbusMessage);
        } else if (dbusMessage.hasMemberName("InterfacesRemoved")) {
            onInterfacesRemovedSignal(dbusMessage);
        }

        return CommonAPI::SubscriptionStatus::RETAIN;
    }

    virtual ~DBusInstanceAvailabilityStatusChangedEvent() {
        dbusProxy_.removeSignalMemberHandler(interfacesAddedSubscription_);
        dbusProxy_.removeSignalMemberHandler(interfacesRemovedSubscription_);
    }

 protected:
    virtual void onFirstListenerAdded(const CancellableListener&) {
        interfacesAddedSubscription_ = dbusProxy_.addSignalMemberHandler(
                        dbusProxy_.getDBusObjectPath(),
                        DBusObjectManagerStub::getInterfaceName(),
                        "InterfacesAdded",
                        "oa{sa{sv}}",
                        this);

        interfacesRemovedSubscription_ = dbusProxy_.addSignalMemberHandler(
                        dbusProxy_.getDBusObjectPath(),
                        DBusObjectManagerStub::getInterfaceName(),
                        "InterfacesRemoved",
                        "oas",
                        this);
    }

    virtual void onLastListenerRemoved(const CancellableListener&) {
        dbusProxy_.removeSignalMemberHandler(interfacesAddedSubscription_);
        dbusProxy_.removeSignalMemberHandler(interfacesRemovedSubscription_);
    }

 private:
    inline void onInterfacesAddedSignal(const DBusMessage& dbusMessage) {
        DBusInputStream dbusInputStream(dbusMessage);
        std::string dbusObjectPath;
        DBusObjectManagerStub::DBusInterfacesAndPropertiesDict dbusInterfacesAndPropertiesDict;

        dbusInputStream >> dbusObjectPath;
        assert(!dbusInputStream.hasError());

        dbusInputStream >> dbusInterfacesAndPropertiesDict;
        assert(!dbusInputStream.hasError());

        for (const auto& dbusInterfaceIterator : dbusInterfacesAndPropertiesDict) {
            const std::string& dbusInterfaceName = dbusInterfaceIterator.first;

            if(dbusInterfaceName == observedInterfaceName_) {
                notifyInterfaceStatusChanged(dbusObjectPath, dbusInterfaceName, AvailabilityStatus::AVAILABLE);
            }
        }
    }

    inline void onInterfacesRemovedSignal(const DBusMessage& dbusMessage) {
        DBusInputStream dbusInputStream(dbusMessage);
        std::string dbusObjectPath;
        std::vector<std::string> dbusInterfaceNames;

        dbusInputStream >> dbusObjectPath;
        assert(!dbusInputStream.hasError());

        dbusInputStream >> dbusInterfaceNames;
        assert(!dbusInputStream.hasError());

        for (const auto& dbusInterfaceName : dbusInterfaceNames) {
            if(dbusInterfaceName == observedInterfaceName_) {
                notifyInterfaceStatusChanged(dbusObjectPath, dbusInterfaceName, AvailabilityStatus::NOT_AVAILABLE);
            }
        }
    }

    void notifyInterfaceStatusChanged(const std::string& dbusObjectPath,
                                      const std::string& dbusInterfaceName,
                                      const AvailabilityStatus& availabilityStatus) {
        std::string commonApiAddress;

        DBusAddressTranslator::getInstance().searchForCommonAddress(
                        dbusInterfaceName,
                        dbusProxy_.getDBusBusName(),
                        dbusObjectPath,
                        commonApiAddress);

        notifyListeners(commonApiAddress, availabilityStatus);
    }


    DBusProxy& dbusProxy_;
    std::string observedInterfaceName_;
    DBusProxyConnection::DBusSignalHandlerToken interfacesAddedSubscription_;
    DBusProxyConnection::DBusSignalHandlerToken interfacesRemovedSubscription_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_INSTANCE_AVAILABILITY_STATUS_CHANGED_EVENT_H_
