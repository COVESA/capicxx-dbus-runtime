/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "FakeBinding.h"


const CommonAPI::MiddlewareInfo middlewareInfo("Fake", &CommonAPI::Fake::FakeRuntime::getInstance, {55, 4} );


namespace CommonAPI {
namespace Fake {

const MiddlewareInfo FakeRuntime::middlewareInfo_ = middlewareInfo;

__attribute__((constructor)) void registerFakeMiddleware(void) {
    Runtime::registerRuntimeLoader("Fake", &FakeRuntime::getInstance);
}

FakeFactory::FakeFactory(std::shared_ptr<Runtime> runtime, const MiddlewareInfo* middlewareInfo, std::shared_ptr<MainLoopContext> mainLoopContext):
                CommonAPI::Factory(runtime, middlewareInfo) {}

std::vector<std::string> FakeFactory::getAvailableServiceInstances(const std::string& serviceName, const std::string& serviceDomainName) {
    return std::vector<std::string>();
}

bool FakeFactory::isServiceInstanceAlive(const std::string& serviceAddress) {
    return false;
}

bool FakeFactory::isServiceInstanceAlive(const std::string& serviceInstanceID, const std::string& serviceName, const std::string& serviceDomainName) {
    return false;
}

void FakeFactory::getAvailableServiceInstancesAsync(GetAvailableServiceInstancesCallback callback, const std::string& serviceName, const std::string& serviceDomainName) {}
void FakeFactory::isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceAddress) {}
void FakeFactory::isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceInstanceID, const std::string& serviceName, const std::string& serviceDomainName) {}

std::shared_ptr<Proxy> FakeFactory::createProxy(const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain) {
    return std::shared_ptr<Proxy>(NULL);
}

bool FakeFactory::registerAdapter(std::shared_ptr<StubBase> stubBase, const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain) {
    return false;
}

bool FakeFactory::unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain) {
    return false;
}

std::shared_ptr<FakeServicePublisher> FakeServicePublisher::getInstance() {
    static std::shared_ptr<FakeServicePublisher> instance;
    if(!instance) {
        instance = std::make_shared<FakeServicePublisher>();
    }
    return instance;
}

bool FakeServicePublisher::registerService(const std::string& serviceAddress, std::shared_ptr<FakeStubAdapter> adapter) {
    return false;
}

bool FakeServicePublisher::unregisterService(const std::string& serviceAddress) {
    return false;
}

std::shared_ptr<Runtime> FakeRuntime::getInstance() {
    static std::shared_ptr<Runtime> singleton_;
    if(!singleton_) {
        singleton_ = std::make_shared<FakeRuntime>();
    }
    return singleton_;
}

std::shared_ptr<Factory> FakeRuntime::doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
                                                      const std::string& factoryName,
                                                      const bool nullOnInvalidName) {
    auto factory = std::make_shared<FakeFactory>(this->shared_from_this(), &middlewareInfo_, mainLoopContext);
    return factory;
}

std::shared_ptr<ServicePublisher> FakeRuntime::getServicePublisher() {
    return FakeServicePublisher::getInstance();
}

} // namespace Fake
} // namespace CommonAPI
