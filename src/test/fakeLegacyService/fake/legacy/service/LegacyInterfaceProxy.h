/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_PROXY_H_
#define FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_PROXY_H_

#include "LegacyInterfaceProxyBase.h"

namespace fake {
namespace legacy {
namespace service {

template <typename ... _AttributeExtensions>
class LegacyInterfaceProxy: virtual public LegacyInterface, virtual public LegacyInterfaceProxyBase, public _AttributeExtensions... {
 public:
    LegacyInterfaceProxy(std::shared_ptr<CommonAPI::Proxy> delegate);
    ~LegacyInterfaceProxy();




    virtual void TestMethod(const int32_t& input, CommonAPI::CallStatus& callStatus, int32_t& val1, int32_t& val2);
    virtual std::future<CommonAPI::CallStatus> TestMethodAsync(const int32_t& input, TestMethodAsyncCallback callback);

    virtual void OtherTestMethod(CommonAPI::CallStatus& callStatus, std::string& greeting, int32_t& identifier);
    virtual std::future<CommonAPI::CallStatus> OtherTestMethodAsync(OtherTestMethodAsyncCallback callback);

    virtual void finish(CommonAPI::CallStatus& callStatus);
    virtual std::future<CommonAPI::CallStatus> finishAsync(FinishAsyncCallback callback);

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;
    virtual bool isAvailable() const;
    virtual CommonAPI::ProxyStatusEvent& getProxyStatusEvent();
    virtual CommonAPI::InterfaceVersionAttribute& getInterfaceVersionAttribute();

 private:
    std::shared_ptr<LegacyInterfaceProxyBase> delegate_;
};


//
// LegacyInterfaceProxy Implementation
//
template <typename ... _AttributeExtensions>
LegacyInterfaceProxy<_AttributeExtensions...>::LegacyInterfaceProxy(std::shared_ptr<CommonAPI::Proxy> delegate):
        delegate_(std::dynamic_pointer_cast<LegacyInterfaceProxyBase>(delegate)),
        _AttributeExtensions(*(std::dynamic_pointer_cast<LegacyInterfaceProxyBase>(delegate)))... {
}

template <typename ... _AttributeExtensions>
LegacyInterfaceProxy<_AttributeExtensions...>::~LegacyInterfaceProxy() {
}



template <typename ... _AttributeExtensions>
void LegacyInterfaceProxy<_AttributeExtensions...>::TestMethod(const int32_t& input, CommonAPI::CallStatus& callStatus, int32_t& val1, int32_t& val2) {
    delegate_->TestMethod(input, callStatus, val1, val2);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> LegacyInterfaceProxy<_AttributeExtensions...>::TestMethodAsync(const int32_t& input, TestMethodAsyncCallback callback) {
    return delegate_->TestMethodAsync(input, callback);
}

template <typename ... _AttributeExtensions>
void LegacyInterfaceProxy<_AttributeExtensions...>::OtherTestMethod(CommonAPI::CallStatus& callStatus, std::string& greeting, int32_t& identifier) {
    delegate_->OtherTestMethod(callStatus, greeting, identifier);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> LegacyInterfaceProxy<_AttributeExtensions...>::OtherTestMethodAsync(OtherTestMethodAsyncCallback callback) {
    return delegate_->OtherTestMethodAsync(callback);
}

template <typename ... _AttributeExtensions>
void LegacyInterfaceProxy<_AttributeExtensions...>::finish(CommonAPI::CallStatus& callStatus) {
    delegate_->finish(callStatus);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> LegacyInterfaceProxy<_AttributeExtensions...>::finishAsync(FinishAsyncCallback callback) {
    return delegate_->finishAsync(callback);
}


template <typename ... _AttributeExtensions>
std::string LegacyInterfaceProxy<_AttributeExtensions...>::getAddress() const {
    return delegate_->getAddress();
}

template <typename ... _AttributeExtensions>
const std::string& LegacyInterfaceProxy<_AttributeExtensions...>::getDomain() const {
    return delegate_->getDomain();
}

template <typename ... _AttributeExtensions>
const std::string& LegacyInterfaceProxy<_AttributeExtensions...>::getServiceId() const {
    return delegate_->getServiceId();
}

template <typename ... _AttributeExtensions>
const std::string& LegacyInterfaceProxy<_AttributeExtensions...>::getInstanceId() const {
    return delegate_->getInstanceId();
}

template <typename ... _AttributeExtensions>
bool LegacyInterfaceProxy<_AttributeExtensions...>::isAvailable() const {
    return delegate_->isAvailable();
}

template <typename ... _AttributeExtensions>
CommonAPI::ProxyStatusEvent& LegacyInterfaceProxy<_AttributeExtensions...>::getProxyStatusEvent() {
    return delegate_->getProxyStatusEvent();
}

template <typename ... _AttributeExtensions>
CommonAPI::InterfaceVersionAttribute& LegacyInterfaceProxy<_AttributeExtensions...>::getInterfaceVersionAttribute() {
    return delegate_->getInterfaceVersionAttribute();
}

} // namespace service
} // namespace legacy
} // namespace fake



#endif // FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_PROXY_H_
