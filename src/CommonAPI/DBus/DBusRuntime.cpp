/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusRuntime.h"

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
    auto factory = std::make_shared<DBusFactory>(this->shared_from_this(), &middlewareInfo_, mainLoopContext);
    return factory;
}

} // namespace DBus
} // namespace CommonAPI
