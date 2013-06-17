/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusRuntime.h"
#include "DBusAddressTranslator.h"

namespace CommonAPI {
namespace DBus {

const MiddlewareInfo DBusRuntime::middlewareInfo_("DBus", &DBusRuntime::getInstance);

__attribute__((constructor)) void registerDBusMiddleware(void) {
    Runtime::registerRuntimeLoader("DBus", &DBusRuntime::getInstance);
}

std::shared_ptr<Runtime> DBusRuntime::getInstance() {
    static std::shared_ptr<Runtime> singleton_;
    if(!singleton_) {
        singleton_ = std::make_shared<DBusRuntime>();
    }
    return singleton_;
}

std::shared_ptr<Factory> DBusRuntime::createFactory(std::shared_ptr<MainLoopContext> mainLoopContext) {
    return createFactory(mainLoopContext, "");
}

std::shared_ptr<Factory> DBusRuntime::createFactory(const std::string factoryName,
                                                       const bool nullOnInvalidName) {
    return createFactory(std::shared_ptr<MainLoopContext>(NULL), factoryName, nullOnInvalidName);
}



std::shared_ptr<Factory> DBusRuntime::createFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
                                                       const std::string factoryName,
                                                       const bool nullOnInvalidName) {
    auto factory = std::shared_ptr<DBusFactory>(NULL);

    if (factoryName == "")
        factory = std::make_shared<DBusFactory>(this->shared_from_this(), &middlewareInfo_, mainLoopContext);
    else
    {
        DBusAddressTranslator::FactoryConfigDBus* factoryConfigDBus =
                        DBusAddressTranslator::getInstance().searchForFactoryConfiguration(factoryName);
        DBusAddressTranslator::FactoryConfigDBus defaultFactoryConfigDBus;

        if (factoryConfigDBus == NULL)
        {
            // unknown / unconfigured Factory requested
            if (nullOnInvalidName)
                return (NULL);
            else
            {
                DBusFactory::getDefaultFactoryConfig(defaultFactoryConfigDBus); // get default settings
                factoryConfigDBus = &defaultFactoryConfigDBus;
            }
        }

        factory = std::make_shared<DBusFactory>(
                        this->shared_from_this(),
                        &middlewareInfo_,
                        *factoryConfigDBus,
                        mainLoopContext);
    }

    return factory;
}

std::shared_ptr<ServicePublisher> DBusRuntime::getServicePublisher() {
    return DBusServicePublisher::getInstance();
}

} // namespace DBus
} // namespace CommonAPI
