/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_PROXY_MANAGER_H_
#define COMMONAPI_DBUS_DBUS_PROXY_MANAGER_H_

#include <CommonAPI/ProxyManager.h>

#include "DBusProxy.h"
#include "DBusFactory.h"
#include "DBusObjectManagerStub.h"
#include "DBusInstanceAvailabilityStatusChangedEvent.h"

#include <functional>
#include <future>
#include <string>
#include <vector>


namespace CommonAPI {
namespace DBus {

class DBusProxyManager: public ProxyManager {
 public:
    DBusProxyManager(DBusProxy& dbusProxy,
                     const std::string& interfaceName,
                     const std::shared_ptr<DBusFactory> factory);

    virtual void getAvailableInstances(CommonAPI::CallStatus&, std::vector<std::string>& availableInstances);
	virtual std::future<CallStatus> getAvailableInstancesAsync(GetAvailableInstancesCallback callback);

    virtual void getInstanceAvailabilityStatus(const std::string& instanceAddress,
                                               CallStatus& callStatus,
                                               AvailabilityStatus& availabilityStatus);

    virtual std::future<CallStatus> getInstanceAvailabilityStatusAsync(const std::string&,
                                                               GetInstanceAvailabilityStatusCallback callback);

    virtual InstanceAvailabilityStatusChangedEvent& getInstanceAvailabilityStatusChangedEvent();
 protected:
    virtual std::shared_ptr<Proxy> createProxy(const std::string& instanceId);
 private:

    void instancesAsyncCallback(const CommonAPI::CallStatus& status,
                                const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dict,
                                GetAvailableInstancesCallback& call);

    SubscriptionStatus instanceAliveAsyncCallback(const AvailabilityStatus& alive,
                                    GetInstanceAvailabilityStatusCallback& call,
                                    std::shared_ptr<std::promise<CallStatus> >& callStatus);

    void translateCommonApiAddresses(const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dbusObjectPathAndInterfacesDict,
                                     std::vector<std::string>& instanceIds);

    DBusProxy& dbusProxy_;
    DBusInstanceAvailabilityStatusChangedEvent dbusInstanceAvailabilityStatusEvent_;
    const std::shared_ptr<DBusFactory> factory_;
    const std::shared_ptr<DBusServiceRegistry> registry_;

    const std::string interfaceId_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_MANAGER_H_
