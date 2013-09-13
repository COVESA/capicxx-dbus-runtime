/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/Configuration.h>
#include <CommonAPI/utils.h>

#include "DBusRuntime.h"
#include "DBusAddressTranslator.h"


const char middlewareIdentifier[] = "DBus";

const CommonAPI::MiddlewareInfo middlewareInfo(middlewareIdentifier,
                                               &CommonAPI::DBus::DBusRuntime::getInstance,
                                               {COMMONAPI_DBUS_VERSION_MAJOR, COMMONAPI_DBUS_VERSION_MINOR} );


namespace CommonAPI {
namespace DBus {

__attribute__((constructor)) void registerDBusMiddleware(void) {
    Runtime::registerRuntimeLoader(middlewareIdentifier, &DBusRuntime::getInstance);
}

std::shared_ptr<Runtime> DBusRuntime::getInstance() {
    static std::shared_ptr<Runtime> dbusRuntimeSingleton_;
    if (!dbusRuntimeSingleton_) {
        dbusRuntimeSingleton_ = std::make_shared<DBusRuntime>();
        for (const auto& genericLibraryPath: Configuration::getInstance().getGenericLibraryPaths(middlewareIdentifier)) {
            if (!loadGenericLibrary(middlewareIdentifier, genericLibraryPath, false)) {
                return std::shared_ptr<Runtime>(NULL);
            }
        }
        findAndLoadGenericLibraries(middlewareIdentifier, Configuration::getInstance().getLibrarySearchPaths());
    }
    return dbusRuntimeSingleton_;
}


std::shared_ptr<Factory> DBusRuntime::doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
                                                      const std::string& factoryName,
                                                      const bool nullOnInvalidName) {
    auto factory = std::shared_ptr<DBusFactory>(NULL);

    if (factoryName == "") {
        factory = std::make_shared<DBusFactory>(this->shared_from_this(), &middlewareInfo, mainLoopContext);
    } else {
        const DBusFactoryConfig* dbusFactoryConfig = DBusConfiguration::getInstance().getFactoryConfiguration(factoryName);

        if (dbusFactoryConfig == NULL) {
            if (nullOnInvalidName) {
                return std::shared_ptr<Factory>(NULL);
            } else {
                dbusFactoryConfig = new DBusFactoryConfig();
            }
        }

        factory = std::make_shared<DBusFactory>(
                        this->shared_from_this(),
                        &middlewareInfo,
                        mainLoopContext,
                        *dbusFactoryConfig);
    }

    return factory;
}

std::shared_ptr<ServicePublisher> DBusRuntime::getServicePublisher() {
    return DBusServicePublisher::getInstance();
}

} // namespace DBus
} // namespace CommonAPI
