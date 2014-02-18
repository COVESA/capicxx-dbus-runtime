/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusProxyManager.h"
#include "DBusAddressTranslator.h"


namespace CommonAPI {
namespace DBus {


DBusProxyManager::DBusProxyManager(DBusProxy& dbusProxy,
                                   const std::string& interfaceId,
                                   const std::shared_ptr<DBusFactory> factory) :
                dbusProxy_(dbusProxy),
                dbusInstanceAvailabilityStatusEvent_(dbusProxy, interfaceId),
                factory_(factory),
                registry_(dbusProxy.getDBusConnection()->getDBusServiceRegistry()),
                interfaceId_(interfaceId)

{ }

void DBusProxyManager::instancesAsyncCallback(const CommonAPI::CallStatus& status,
                                              const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dict,
                                              GetAvailableInstancesCallback& call) {
    std::vector<std::string> returnVec;
    if (status == CommonAPI::CallStatus::SUCCESS) {
        translateCommonApiAddresses(dict, returnVec);
    }
    call(status, returnVec);
}

void DBusProxyManager::getAvailableInstances(CommonAPI::CallStatus& callStatus, std::vector<std::string>& availableInstances) {
    DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;

    DBusProxyHelper<DBusSerializableArguments<>,
                    DBusSerializableArguments<DBusObjectManagerStub::DBusObjectPathAndInterfacesDict> >::callMethodWithReply(
                                    dbusProxy_,
                                    DBusObjectManagerStub::getInterfaceName(),
                                    "GetManagedObjects",
                                    "",
                                    callStatus,
                                    dbusObjectPathAndInterfacesDict);

    if (callStatus == CallStatus::SUCCESS) {
        translateCommonApiAddresses(dbusObjectPathAndInterfacesDict, availableInstances);
    }
}

std::future<CallStatus> DBusProxyManager::getAvailableInstancesAsync(GetAvailableInstancesCallback callback) {
    return CommonAPI::DBus::DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
           CommonAPI::DBus::DBusSerializableArguments<
                   DBusObjectManagerStub::DBusObjectPathAndInterfacesDict> >::callMethodAsync(
        dbusProxy_,
        DBusObjectManagerStub::getInterfaceName(),
        "GetManagedObjects",
        "a{oa{sa{sv}}}",
                    std::move(
                                    std::bind(
                                                    &DBusProxyManager::instancesAsyncCallback,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    callback)));

}

void DBusProxyManager::getInstanceAvailabilityStatus(const std::string& instanceAddress,
                                                     CallStatus& callStatus,
                                                     AvailabilityStatus& availabilityStatus) {

    std::stringstream ss;
    ss << "local:" << interfaceId_ << ":" << instanceAddress;

    std::string interfaceName;
    std::string connectionName;
    std::string objectPath;
    DBusAddressTranslator::getInstance().searchForDBusAddress(
                    ss.str(),
                    interfaceName,
                    connectionName,
                    objectPath);
    availabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
    if (registry_->isServiceInstanceAlive(interfaceName, connectionName, objectPath)) {
        availabilityStatus = AvailabilityStatus::AVAILABLE;
    }
    callStatus = CallStatus::SUCCESS;
}

SubscriptionStatus DBusProxyManager::instanceAliveAsyncCallback(const AvailabilityStatus& alive, GetInstanceAvailabilityStatusCallback& call, std::shared_ptr<std::promise<CallStatus> >& callStatus) {
    call(CallStatus::SUCCESS, alive);
    callStatus->set_value(CallStatus::SUCCESS);
    return SubscriptionStatus::CANCEL;
}

std::future<CallStatus> DBusProxyManager::getInstanceAvailabilityStatusAsync(const std::string& instanceAddress,
                                                                             GetInstanceAvailabilityStatusCallback callback) {
    std::stringstream ss;
    ss << "local:" << interfaceId_ << ":" << instanceAddress;


    std::shared_ptr<std::promise<CallStatus> > promise = std::make_shared<std::promise<CallStatus>>();
    registry_->subscribeAvailabilityListener(
                    ss.str(),
                    std::bind(&DBusProxyManager::instanceAliveAsyncCallback,
                              this,
                              std::placeholders::_1,
                              callback,
                              promise)
    );

    return promise->get_future();
}

DBusProxyManager::InstanceAvailabilityStatusChangedEvent& DBusProxyManager::getInstanceAvailabilityStatusChangedEvent() {
    return dbusInstanceAvailabilityStatusEvent_;
}

void DBusProxyManager::translateCommonApiAddresses(const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dbusObjectPathAndInterfacesDict,
                                                   std::vector<std::string>& instanceIds) {
    for (const auto& dbusObjectPathIter : dbusObjectPathAndInterfacesDict) {
        const std::string& dbusObjectPath = dbusObjectPathIter.first;
        const auto& dbusInterfacesDict = dbusObjectPathIter.second;

        for (const auto& dbusInterfaceIter : dbusInterfacesDict) {
            const std::string dbusInterfaceName = dbusInterfaceIter.first;
            std::string instanceId;

            DBusAddressTranslator::getInstance().searchForCommonAddress(
                            dbusInterfaceName,
                            dbusProxy_.getDBusBusName(),
                            dbusObjectPath,
                            instanceId);

            auto pos = instanceId.find_last_of(':');
            instanceId = instanceId.substr(pos + 1, instanceId.size());

            instanceIds.push_back(instanceId);
        }
    }
}

std::shared_ptr<Proxy> DBusProxyManager::createProxy(const std::string& instanceId) {
    return factory_->createProxy(interfaceId_.c_str(), instanceId, interfaceId_, "local");
}


} // namespace DBus
}// namespace CommonAPI
