/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_FAKE_FAKE_RUNTIME_H_
#define COMMONAPI_FAKE_FAKE_RUNTIME_H_

#include <CommonAPI/CommonAPI.h>


namespace CommonAPI {
namespace Fake {

class FakeFactory: public Factory {
 public:
    FakeFactory(std::shared_ptr<Runtime> runtime, const MiddlewareInfo* middlewareInfo, std::shared_ptr<MainLoopContext> mainLoopContext);
    ~FakeFactory() {}
    std::vector<std::string> getAvailableServiceInstances(const std::string& serviceName, const std::string& serviceDomainName = "local");
    bool isServiceInstanceAlive(const std::string& serviceAddress);
    bool isServiceInstanceAlive(const std::string& serviceInstanceID, const std::string& serviceName, const std::string& serviceDomainName = "local");
    void getAvailableServiceInstancesAsync(GetAvailableServiceInstancesCallback callback, const std::string& serviceName, const std::string& serviceDomainName = "local");
    void isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceAddress);
    void isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceInstanceID, const std::string& serviceName, const std::string& serviceDomainName = "local");
    bool unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain);

 protected:
    std::shared_ptr<Proxy> createProxy(const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain);
    bool registerAdapter(std::shared_ptr<StubBase> stubBase, const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain);
};

class FakeStubAdapter;

class FakeServicePublisher: public ServicePublisher {
public:
   FakeServicePublisher() {}

   static std::shared_ptr<FakeServicePublisher> getInstance();

   bool registerService(const std::string& serviceAddress, std::shared_ptr<FakeStubAdapter> adapter);

   bool unregisterService(const std::string& serviceAddress);
};

class FakeRuntime: public Runtime, public std::enable_shared_from_this<FakeRuntime> {
 public:
    static std::shared_ptr<Runtime> getInstance();

    std::shared_ptr<Factory> doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
                                             const std::string& factoryName,
                                             const bool nullOnInvalidName);

    std::shared_ptr<ServicePublisher> getServicePublisher();

    static const MiddlewareInfo middlewareInfo_;
};

} // namespace Fake
} // namespace CommonAPI


extern "C" const CommonAPI::MiddlewareInfo middlewareInfo;

#endif // COMMONAPI_FAKE_FAKE_RUNTIME_H_
