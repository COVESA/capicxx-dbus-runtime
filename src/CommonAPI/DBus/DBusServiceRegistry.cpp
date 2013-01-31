/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <utility>
#include <sstream>
#include <string>
#include <tuple>
#include <unistd.h>

#include "DBusServiceRegistry.h"
#include "DBusInputStream.h"
#include "DBusDaemonProxy.h"
#include "DBusConnection.h"
#include "DBusUtils.h"


namespace CommonAPI {
namespace DBus {


DBusServiceRegistry::DBusServiceRegistry(std::shared_ptr<DBusConnection> dbusConnection) :
                dbusConnection_(dbusConnection),
                ready(false),
                serviceStatusEvent_(std::shared_ptr<DBusServiceRegistry>(this)),
                readyPromise_(),
                readyMutex_()
{
    readyFuture_ = readyPromise_.get_future();
    cacheAllServices();
    dbusNameOwnerChangedEventSubscription_ =
                    dbusConnection_->getDBusDaemonProxy()->getNameOwnerChangedEvent().subscribe(
                                    std::bind(&DBusServiceRegistry::onDBusNameOwnerChangedEvent,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3));
    std::thread(std::bind(&DBusServiceRegistry::isReadyBlocking, this)).detach();
}

void DBusServiceRegistry::registerAvailabilityListener(const std::string& service, const std::function<void(bool)>& listener) {
    availabilityCallbackList.insert({service, listener});

}

DBusServiceStatusEvent& DBusServiceRegistry::getServiceStatusEvent() {
    return serviceStatusEvent_;
}

DBusServiceRegistry::~DBusServiceRegistry() {
    dbusConnection_->getDBusDaemonProxy()->getNameOwnerChangedEvent().unsubscribe(dbusNameOwnerChangedEventSubscription_);
}

std::future<bool>& DBusServiceRegistry::getReadyFuture() {
    return readyFuture_;
}

bool DBusServiceRegistry::isReadyBlocking() {
    if (!ready) {
        readyMutex_.lock();
        auto status = readyFuture_.wait_for(std::chrono::seconds(1));
        if (checkReady(status)) {
            ready = true;
        } else {
            ready = true;
            readyPromise_.set_value(true);
        }
        readyMutex_.unlock();
    }
    return ready;
}

bool DBusServiceRegistry::isReady() {
	return ready;
}

std::vector<std::string> DBusServiceRegistry::getAvailableServiceInstances(const std::string& serviceInterfaceName,
                                                                           const std::string& serviceDomainName) {
	if (!isReadyBlocking()) {
		return std::vector<std::string>();
	}

    if (serviceDomainName != "local" || !dbusConnection_->isConnected()) {
        return std::vector<std::string>();
    }

    std::vector<std::string> addressesOfKnownServiceInstances;
    auto knownServiceInstancesIteratorPair = dbusCachedProvidersForInterfaces_.equal_range(serviceInterfaceName);

    while(knownServiceInstancesIteratorPair.first != knownServiceInstancesIteratorPair.second) {
        const DBusServiceInstanceId dbusServiceInstanceId = knownServiceInstancesIteratorPair.first->second;
        addressesOfKnownServiceInstances.push_back(findInstanceIdMapping(dbusServiceInstanceId));
        ++knownServiceInstancesIteratorPair.first;
    }

    return addressesOfKnownServiceInstances;
}

void DBusServiceRegistry::onManagedPathsList(const CallStatus& status, DBusObjectToInterfaceDict managedObjects,
        std::list<std::string>::iterator iter, std::shared_ptr<std::list<std::string>> list) {

    auto objectPathIterator = managedObjects.begin();

    while (objectPathIterator != managedObjects.end()) {
        const std::string& serviceObjPath = objectPathIterator->first;
        auto interfaceNameIterator = objectPathIterator->second.begin();

        while (interfaceNameIterator != objectPathIterator->second.end()) {
            const std::string& interfaceName = interfaceNameIterator->first;
            dbusCachedProvidersForInterfaces_.insert( { interfaceName, { *iter, serviceObjPath } });
            ++interfaceNameIterator;
        }
        ++objectPathIterator;
    }

    list->erase(iter);

    if (list->size() == 0) {
        readyMutex_.lock();
        if (!ready) {
            readyPromise_.set_value(true);
            ready = true;
        }
        readyMutex_.unlock();
    }
}

bool DBusServiceRegistry::isServiceInstanceAlive(const std::string& address) {
	std::vector<std::string> parts = split(address, ':');
	return isServiceInstanceAlive(parts[2], parts[1], parts[0]);
}


bool DBusServiceRegistry::isServiceInstanceAlive(const std::string& serviceInstanceID,
                                                 const std::string& serviceInterfaceName,
                                                 const std::string& serviceDomainName ) {
	if (!isReadyBlocking()) {
		return false;
	}

    if (serviceDomainName != "local" || !dbusConnection_->isConnected()) {
        return false;
    }

    DBusServiceInstanceId serviceInstanceId = findInstanceIdMapping(serviceInstanceID);

    auto knownInstancesForInterfaceIteratorPair = dbusCachedProvidersForInterfaces_.equal_range(serviceInterfaceName);

    while(knownInstancesForInterfaceIteratorPair.first != knownInstancesForInterfaceIteratorPair.second) {
        DBusServiceInstanceId knownServiceId = knownInstancesForInterfaceIteratorPair.first->second;
        if(knownServiceId == serviceInstanceId) {
            return true;
        }
        ++knownInstancesForInterfaceIteratorPair.first;
    }

    return false;
}

void DBusServiceRegistry::getManagedObjects(const std::string& dbusWellKnownBusName) {
    auto callMessage = DBusMessage::createMethodCall(
                    dbusWellKnownBusName.c_str(),
                    "/",
                    "org.freedesktop.DBus.ObjectManager",
                    "GetManagedObjects",
                    "");
    dbusConnection_->sendDBusMessageWithReplyAsync(
                    callMessage,
                    DBusProxyAsyncCallbackHandler<DBusObjectToInterfaceDict>::create(
                                    std::bind(
                                                    &DBusServiceRegistry::onManagedPaths,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    dbusWellKnownBusName)), 100);

}

void DBusServiceRegistry::onManagedPaths(const CallStatus& status, DBusObjectToInterfaceDict managedObjects,
		std::string dbusWellKnownBusName) {

	auto objectPathIterator = managedObjects.begin();

	while (objectPathIterator != managedObjects.end()) {
		const std::string& serviceObjPath = objectPathIterator->first;
		auto interfaceNameIterator = objectPathIterator->second.begin();

		while (interfaceNameIterator != objectPathIterator->second.end()) {
			const std::string& interfaceName = interfaceNameIterator->first;
			dbusCachedProvidersForInterfaces_.insert( { interfaceName, { dbusWellKnownBusName, serviceObjPath } });
			updateListeners(dbusWellKnownBusName, serviceObjPath, interfaceName, true);
			++interfaceNameIterator;
		}

		++objectPathIterator;
	}
}

void DBusServiceRegistry::updateListeners(const std::string& conName, const std::string& objName, const std::string& intName , bool available) {
    std::string commonAPIAddress = DBusAddressTranslator::getInstance().findCommonAPIAddressForDBusAddress(conName, objName, intName);
    auto found = availabilityCallbackList.equal_range(std::move(commonAPIAddress));
    auto foundIter = found.first;
    while (foundIter != found.second) {
        foundIter->second(true);
        foundIter++;
    }

}

void DBusServiceRegistry::addProvidedServiceInstancesToCache(const std::string& dbusNames) {
	getManagedObjects(dbusNames);
}

void DBusServiceRegistry::addProvidedServiceInstancesToCache(std::vector<std::string>& dbusNames) {

    std::shared_ptr<std::list<std::string>> dbusList = std::make_shared<std::list<std::string>>(dbusNames.begin(), dbusNames.end());

    auto iter = dbusList->begin();

    while (iter != dbusList->end()) {

            auto callMessage = DBusMessage::createMethodCall(
                            iter->c_str(),
                            "/",
                            "org.freedesktop.DBus.ObjectManager",
                            "GetManagedObjects",
                            "");
            dbusConnection_->sendDBusMessageWithReplyAsync(
                            callMessage,
                            DBusProxyAsyncCallbackHandler<DBusObjectToInterfaceDict>::create(
                                            std::bind(
                                                            &DBusServiceRegistry::onManagedPathsList,
                                                            this,
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            iter,
                                                            dbusList)), 10);
            iter++;
    }
}


DBusServiceInstanceId DBusServiceRegistry::findInstanceIdMapping(const std::string& instanceId) const {
    DBusServiceInstanceId dbusInstanceId;
    DBusAddressTranslator::getInstance().searchForDBusInstanceId(instanceId, dbusInstanceId.first, dbusInstanceId.second);
    return std::move(dbusInstanceId);
}

std::string DBusServiceRegistry::findInstanceIdMapping(const DBusServiceInstanceId& dbusInstanceId) const {
    std::string instanceId;
    DBusAddressTranslator::getInstance().searchForCommonInstanceId(instanceId, dbusInstanceId.first, dbusInstanceId.second);
    return std::move(instanceId);
}

inline const bool isServiceName(const std::string& name) {
    return name[0] != ':';
}

void DBusServiceRegistry::onDBusNameOwnerChangedEvent(const std::string& affectedName,
                                                      const std::string& oldOwner,
                                                      const std::string& newOwner) {
    if (isServiceName(affectedName)) {
        if(!oldOwner.empty()) {
            removeProvidedServiceInstancesFromCache(affectedName);
        }

        if (!newOwner.empty()) {
            addProvidedServiceInstancesToCache(affectedName);
        }
    }
}


void DBusServiceRegistry::removeProvidedServiceInstancesFromCache(const std::string& dbusWellKnownBusName) {
    auto providersForInterfacesIteratorPair = dbusCachedProvidersForInterfaces_.equal_range(dbusWellKnownBusName);

    //Iteriere über (interfaceName, (serviceInstanceId))
    while(providersForInterfacesIteratorPair.first != providersForInterfacesIteratorPair.second) {

        DBusServiceInstanceId dbusInstanceId = providersForInterfacesIteratorPair.first->second;
        if(dbusInstanceId.first == dbusWellKnownBusName) {
            auto toErase = providersForInterfacesIteratorPair.first;
            ++providersForInterfacesIteratorPair.first;
            dbusCachedProvidersForInterfaces_.erase(toErase);
        }

        ++providersForInterfacesIteratorPair.first;
    }
}

void DBusServiceRegistry::onListNames(const CommonAPI::CallStatus& callStatus, std::vector<std::string> existingBusConnections) {

	if (callStatus == CallStatus::SUCCESS) {
		std::vector<std::string> dbusLivingServiceBusNames;
		for (const std::string& connectionName : existingBusConnections) {
			const bool isWellKnownName = (connectionName[0] != ':');

			if (isWellKnownName) {
				dbusLivingServiceBusNames.push_back(connectionName);
			}
		}
		addProvidedServiceInstancesToCache(dbusLivingServiceBusNames);
	}
}

void DBusServiceRegistry::cacheAllServices() {
    CommonAPI::CallStatus callStatus;
    std::vector<std::string> existingBusConnections;
    dbusConnection_->getDBusDaemonProxy()->listNames(callStatus, existingBusConnections);
    onListNames(callStatus, existingBusConnections);
}


}// namespace DBus
}// namespace CommonAPI
