/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION
#include <CommonAPI/DBus/DBusObjectManagerStub.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusFactory.h>
#include <CommonAPI/DBus/DBusRuntime.h>
#include <CommonAPI/DBus/DBusStubAdapter.h>
#include <CommonAPI/DBus/DBusServicePublisher.h>

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <memory>

static const std::string dbusServiceName = "CommonAPI.DBus.DBusObjectManagerStubTest";
static const std::string& dbusObjectManagerStubPath = "/commonapi/dbus/test/DBusObjectManagerStub";
static const std::string& managedDBusObjectPathPrefix = "/commonapi/dbus/test/DBusObjectManagerStub/ManagedObject";

class TestDBusStubAdapter: public CommonAPI::DBus::DBusStubAdapter {
public:
    TestDBusStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusFactory> factory,
                        const std::string& dbusObjectPath,
                        const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection,
                        const bool isManagingInterface) :
                    DBusStubAdapter(
                                    factory,
                                    "local:" + dbusServiceName + ":commonapi.dbus.tests.TestDBusStubAdapter-" + dbusObjectPath,
                                    "commonapi.dbus.tests.TestDBusStubAdapter",
                                    dbusServiceName,
                                    dbusObjectPath,
                                    dbusConnection,
                                    isManagingInterface) {
    }

    void deactivateManagedInstances() {

    }

    virtual const char* getMethodsDBusIntrospectionXmlData() const {
        return "";
    }

    virtual bool onInterfaceDBusMessage(const CommonAPI::DBus::DBusMessage& dbusMessage) {
        return false;
    }

protected:
    TestDBusStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusFactory> factory,
                        const std::string& dbusObjectPath,
                        const std::string& dbusInterfaceName,
                        const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection,
                        const bool isManagingInterface) :
                    DBusStubAdapter(
                                    factory,
                                    "local:" + dbusServiceName + ":" + dbusInterfaceName + "-" + dbusObjectPath,
                                    dbusInterfaceName,
                                    dbusServiceName,
                                    dbusObjectPath,
                                    dbusConnection,
                                    isManagingInterface) {
    }

};

class ManagerTestDBusStubAdapter: public TestDBusStubAdapter {
public:
    ManagerTestDBusStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusFactory> factory,
                               const std::string& dbusObjectPath,
                               const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection) :
                    TestDBusStubAdapter(
                                    factory,
                                    dbusObjectPath,
                                    "commonapi.dbus.tests.ManagerTestDBusStubAdapter",
                                    dbusConnection,
                                    true) {
    }
};

struct TestDBusObjectManagerSignalHandler: public CommonAPI::DBus::DBusConnection::DBusSignalHandler {
    size_t totalAddedCount;
    size_t totalRemovedCount;

    std::string lastAddedDBusObjectPath;
    std::string lastRemovedDBusObjectPath;

    std::condition_variable signalReceived;
    std::mutex lock;

    ~TestDBusObjectManagerSignalHandler() {
        dbusConnection_->removeSignalMemberHandler(dbusSignalHandlerAddedToken_);
        dbusConnection_->removeSignalMemberHandler(dbusSignalHandlerRemovedToken_);
    }

    virtual CommonAPI::SubscriptionStatus onSignalDBusMessage(const CommonAPI::DBus::DBusMessage& dbusMessage) {
        if (!dbusMessage.hasInterfaceName(CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName())) {
            return CommonAPI::SubscriptionStatus::CANCEL;
        }

        if (!dbusMessage.hasMemberName("InterfacesAdded") && !dbusMessage.hasMemberName("InterfacesRemoved")) {
            return CommonAPI::SubscriptionStatus::CANCEL;
        }

        CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessage);
        std::lock_guard<std::mutex> lockGuard(lock);

        if (dbusMessage.hasMemberName("InterfacesAdded")) {
            totalAddedCount++;
            dbusInputStream >> lastAddedDBusObjectPath;
        } else {
            totalRemovedCount++;
            dbusInputStream >> lastRemovedDBusObjectPath;
        }

        signalReceived.notify_all();

        return CommonAPI::SubscriptionStatus::RETAIN;
    }

    static inline std::shared_ptr<TestDBusObjectManagerSignalHandler> create(
                                                                             const std::string& dbusObjectPath,
                                                                             const std::shared_ptr<
                                                                                             CommonAPI::DBus::DBusProxyConnection>& dbusConnection) {
        auto dbusSignalHandler = new TestDBusObjectManagerSignalHandler(dbusObjectPath, dbusConnection);
        dbusSignalHandler->init();
        return std::shared_ptr<TestDBusObjectManagerSignalHandler>(dbusSignalHandler);
    }

private:
    TestDBusObjectManagerSignalHandler(const std::string& dbusObjectPath,
                                       const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection) :
                    dbusObjectPath_(dbusObjectPath),
                    dbusConnection_(dbusConnection),
                    totalAddedCount(0),
                    totalRemovedCount(0) {
    }

    void init() {
        dbusSignalHandlerAddedToken_ = dbusConnection_->addSignalMemberHandler(
                        dbusObjectPath_,
                        CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName(),
                        "InterfacesAdded",
                        "oa{sa{sv}}",
                        this);

        dbusSignalHandlerRemovedToken_ = dbusConnection_->addSignalMemberHandler(
                        dbusObjectPath_,
                        CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName(),
                        "InterfacesRemoved",
                        "oas",
                        this);
    }

    std::string dbusObjectPath_;
    std::shared_ptr<CommonAPI::DBus::DBusProxyConnection> dbusConnection_;
    CommonAPI::DBus::DBusProxyConnection::DBusSignalHandlerToken dbusSignalHandlerAddedToken_;
    CommonAPI::DBus::DBusProxyConnection::DBusSignalHandlerToken dbusSignalHandlerRemovedToken_;
};

class DBusObjectManagerStubTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        auto runtime = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());
        serviceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        stubDBusConnection_ = serviceFactory->getDbusConnection();
        ASSERT_TRUE(stubDBusConnection_->connect());
        ASSERT_TRUE(bool(stubDBusConnection_->getDBusObjectManager()));
        ASSERT_TRUE(stubDBusConnection_->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        stubDBusConnection_->disconnect();
        stubDBusConnection_.reset();

        proxyDBusConnection_->disconnect();
        proxyDBusConnection_.reset();
    }

    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory;

    void getManagedObjects(const std::string& dbusObjectPath,
                           CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dbusObjectPathAndInterfacesDict) {
        auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                        dbusServiceName,
                        dbusObjectPath,
                        CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName(),
                        "GetManagedObjects");

        CommonAPI::DBus::DBusError dbusError;
        auto dbusMessageReply = proxyDBusConnection_->sendDBusMessageWithReplyAndBlock(dbusMessageCall, dbusError);

        ASSERT_FALSE(dbusError)<< dbusError.getMessage();
        ASSERT_TRUE(dbusMessageReply.isMethodReturnType());
        ASSERT_TRUE(dbusMessageReply.hasSignature("a{oa{sa{sv}}}"));

        CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessageReply);

        dbusInputStream >> dbusObjectPathAndInterfacesDict;
        ASSERT_FALSE(dbusInputStream.hasError());
    }

    void getIntrospectionData(const std::string& dbusObjectPath, std::string& introspectionDataXml) {
        auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                        dbusServiceName,
                        dbusObjectPath,
                        "org.freedesktop.DBus.Introspectable",
                        "Introspect");
        CommonAPI::DBus::DBusError dbusError;
        auto dbusMessageReply = proxyDBusConnection_->sendDBusMessageWithReplyAndBlock(dbusMessageCall, dbusError);

        ASSERT_FALSE(dbusError)<< dbusError.getMessage();
        ASSERT_TRUE(dbusMessageReply.isMethodReturnType());
        ASSERT_TRUE(dbusMessageReply.hasSignature("s"));

        CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessageReply);

        dbusInputStream >> introspectionDataXml;
        ASSERT_FALSE(dbusInputStream.hasError());
    }

    void waitForInterfacesAdded(const std::shared_ptr<TestDBusObjectManagerSignalHandler>& dbusSignalHandler,
                                const size_t& interfacesAddedCount,
                                const size_t& interfacesRemovedExpectedCount) {
        const size_t waitMillisecondsPerInterface = 300;

        std::unique_lock<std::mutex> lock(dbusSignalHandler->lock);
        auto waitResult = dbusSignalHandler->signalReceived.wait_for(
                        lock,
                        std::chrono::milliseconds(interfacesAddedCount * waitMillisecondsPerInterface),
                        [&]() {return dbusSignalHandler->totalAddedCount == interfacesAddedCount;});
        ASSERT_TRUE(waitResult);
        ASSERT_EQ(dbusSignalHandler->totalRemovedCount, interfacesRemovedExpectedCount);

        const std::string lastAddedDBusObjectPath = managedDBusObjectPathPrefix
                        + std::to_string(interfacesAddedCount - 1);
        ASSERT_TRUE(dbusSignalHandler->lastAddedDBusObjectPath == lastAddedDBusObjectPath);
    }

    void waitForInterfacesRemoved(const std::shared_ptr<TestDBusObjectManagerSignalHandler>& dbusSignalHandler,
                                  const size_t& interfacesRemovedCount,
                                  const size_t& interfacesAddedExpectedCount) {
        const size_t waitMillisecondsPerInterface = 300;

        std::unique_lock<std::mutex> lock(dbusSignalHandler->lock);
        auto waitResult = dbusSignalHandler->signalReceived.wait_for(
                        lock,
                        std::chrono::milliseconds(interfacesRemovedCount * waitMillisecondsPerInterface),
                        [&]() {return dbusSignalHandler->totalRemovedCount == interfacesRemovedCount;});
        ASSERT_TRUE(waitResult);
        ASSERT_EQ(dbusSignalHandler->totalAddedCount, interfacesAddedExpectedCount);

        const std::string lastRemovedDBusObjectPath = managedDBusObjectPathPrefix
                        + std::to_string(interfacesRemovedCount - 1);
        ASSERT_TRUE(dbusSignalHandler->lastRemovedDBusObjectPath == lastRemovedDBusObjectPath);
    }

    template<typename _StubType, size_t _ArraySize>
    void createDBusStubAdapterArray(std::array<std::shared_ptr<_StubType>, _ArraySize>& dbusStubAdapter) {
        for (size_t i = 0; i < _ArraySize; i++) {
            dbusStubAdapter[i] = std::make_shared<_StubType>(serviceFactory,
                            managedDBusObjectPathPrefix + std::to_string(i),
                            stubDBusConnection_,
                            false);

            ASSERT_TRUE(bool(dbusStubAdapter[i]));

            dbusStubAdapter[i]->init();
        }
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;
};

TEST_F(DBusObjectManagerStubTest, EmptyRootGetManagedObjectsWorks) {
    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;

    getManagedObjects("/", dbusObjectPathAndInterfacesDict);

    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusObjectManagerStubTest, RootObjectManagerIntrospectionWorks) {
    std::string introspectionDataXml;

    getIntrospectionData("/", introspectionDataXml);

    ASSERT_FALSE(introspectionDataXml.empty());
    ASSERT_TRUE(introspectionDataXml.find("GetManagedObjects") != std::string::npos);
    ASSERT_TRUE(introspectionDataXml.find("InterfacesAdded") != std::string::npos);
    ASSERT_TRUE(introspectionDataXml.find("InterfacesRemoved") != std::string::npos);
}

TEST_F(DBusObjectManagerStubTest, RootRegisterStubAdapterWorks) {
    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    auto dbusSignalHandler = TestDBusObjectManagerSignalHandler::create("/", proxyDBusConnection_);
    std::array<std::shared_ptr<TestDBusStubAdapter>, 10> dbusStubAdapterArray;

    createDBusStubAdapterArray(dbusStubAdapterArray);

    const bool isServiceRegistrationSuccessful = std::all_of(
                    dbusStubAdapterArray.begin(),
                    dbusStubAdapterArray.end(),
                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                        return CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(dbusStubAdapter);
                    });
    ASSERT_TRUE(isServiceRegistrationSuccessful);

    waitForInterfacesAdded(dbusSignalHandler, dbusStubAdapterArray.size(), 0);

    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_EQ(dbusObjectPathAndInterfacesDict.size(), dbusStubAdapterArray.size());

    const bool isServiceDeregistrationSuccessful = std::all_of(
                    dbusStubAdapterArray.begin(),
                    dbusStubAdapterArray.end(),
                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                        const auto& serviceAddress = dbusStubAdapter->getAddress();
                        return CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(serviceAddress);
                    });
    ASSERT_TRUE(isServiceDeregistrationSuccessful);

    waitForInterfacesRemoved(dbusSignalHandler, dbusStubAdapterArray.size(), dbusStubAdapterArray.size());

    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}


TEST_F(DBusObjectManagerStubTest, RegisterManagerStubAdapterWorks) {
    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    auto managerDBusStubAdapter = std::make_shared<ManagerTestDBusStubAdapter>(
                    serviceFactory,
                    dbusObjectManagerStubPath,
                    stubDBusConnection_);
    managerDBusStubAdapter->init();

    ASSERT_TRUE(CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(managerDBusStubAdapter));

    getManagedObjects("/", dbusObjectPathAndInterfacesDict);

    ASSERT_FALSE(dbusObjectPathAndInterfacesDict.empty());
    ASSERT_EQ(dbusObjectPathAndInterfacesDict.size(), 1);
    ASSERT_EQ(dbusObjectPathAndInterfacesDict.count(dbusObjectManagerStubPath), 1);

    ASSERT_EQ(dbusObjectPathAndInterfacesDict[dbusObjectManagerStubPath].size(), 2);
    ASSERT_EQ(dbusObjectPathAndInterfacesDict[dbusObjectManagerStubPath].count(
                                CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName()),
              1);
    ASSERT_EQ(dbusObjectPathAndInterfacesDict[dbusObjectManagerStubPath].count(
                                managerDBusStubAdapter->getInterfaceName()),
              1);

    ASSERT_TRUE(
                    CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(
                                    managerDBusStubAdapter->getAddress()));

    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusObjectManagerStubTest, ManagerStubAdapterExportAndUnexportWorks) {
    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    auto dbusSignalHandler = TestDBusObjectManagerSignalHandler::create(
                    dbusObjectManagerStubPath,
                    proxyDBusConnection_);
    auto managerDBusStubAdapter = std::make_shared<ManagerTestDBusStubAdapter>(
                    serviceFactory,
                    dbusObjectManagerStubPath,
                    stubDBusConnection_);
    managerDBusStubAdapter->init();

    ASSERT_TRUE(CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(managerDBusStubAdapter));

    std::array<std::shared_ptr<TestDBusStubAdapter>, 10> dbusStubAdapterArray;
    createDBusStubAdapterArray(dbusStubAdapterArray);

    const bool isServiceRegistrationSuccessful =
                    std::all_of(
                                    dbusStubAdapterArray.begin(),
                                    dbusStubAdapterArray.end(),
                                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                                        return CommonAPI::DBus::DBusServicePublisher::getInstance()->registerManagedService(dbusStubAdapter);
                                    });
    ASSERT_TRUE(isServiceRegistrationSuccessful);

    const bool isServiceExportSuccessful =
                    std::all_of(
                                    dbusStubAdapterArray.begin(),
                                    dbusStubAdapterArray.end(),
                                    [&](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                                        return stubDBusConnection_->getDBusObjectManager()->exportManagedDBusStubAdapter(managerDBusStubAdapter->getObjectPath(), dbusStubAdapter);
                                    });
    ASSERT_TRUE(isServiceExportSuccessful);

    waitForInterfacesAdded(dbusSignalHandler, dbusStubAdapterArray.size(), 0);

    getManagedObjects(dbusObjectManagerStubPath, dbusObjectPathAndInterfacesDict);
    EXPECT_EQ(dbusObjectPathAndInterfacesDict.size(), dbusStubAdapterArray.size());

    const bool isServiceUnexportSuccessful =
                    std::all_of(
                                    dbusStubAdapterArray.begin(),
                                    dbusStubAdapterArray.end(),
                                    [&](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                                        return stubDBusConnection_->getDBusObjectManager()->unexportManagedDBusStubAdapter(managerDBusStubAdapter->getObjectPath(), dbusStubAdapter);
                                    });
    ASSERT_TRUE(isServiceUnexportSuccessful);

    const bool isServiceDeregistrationSuccessful = std::all_of(
                    dbusStubAdapterArray.begin(),
                    dbusStubAdapterArray.end(),
                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                        return CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterManagedService(
                                        dbusStubAdapter->getAddress());
                    });
    ASSERT_TRUE(isServiceDeregistrationSuccessful);

    waitForInterfacesRemoved(dbusSignalHandler, dbusStubAdapterArray.size(), dbusStubAdapterArray.size());

    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects(dbusObjectManagerStubPath, dbusObjectPathAndInterfacesDict);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    ASSERT_TRUE(CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(managerDBusStubAdapter->getAddress()));
}


TEST_F(DBusObjectManagerStubTest, DestructorUnpublishingWorks) {
    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    auto dbusSignalHandler = TestDBusObjectManagerSignalHandler::create(
                    dbusObjectManagerStubPath,
                    proxyDBusConnection_);
    auto managerDBusStubAdapter = std::make_shared<ManagerTestDBusStubAdapter>(
                    serviceFactory,
                    dbusObjectManagerStubPath,
                    stubDBusConnection_);
    managerDBusStubAdapter->init();

    EXPECT_TRUE(CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(managerDBusStubAdapter));

    std::array<std::shared_ptr<TestDBusStubAdapter>, 10> dbusStubAdapterArray;
    createDBusStubAdapterArray(dbusStubAdapterArray);

    const bool isServiceRegistrationSuccessful =
                    std::all_of(
                                    dbusStubAdapterArray.begin(),
                                    dbusStubAdapterArray.end(),
                                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                                        return CommonAPI::DBus::DBusServicePublisher::getInstance()->registerManagedService(dbusStubAdapter);
                                    });
    ASSERT_TRUE(isServiceRegistrationSuccessful);

    const bool isServiceExportSuccessful =
                    std::all_of(
                                    dbusStubAdapterArray.begin(),
                                    dbusStubAdapterArray.end(),
                                    [&](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                                        return stubDBusConnection_->getDBusObjectManager()->exportManagedDBusStubAdapter(managerDBusStubAdapter->getObjectPath(), dbusStubAdapter);
                                    });
    ASSERT_TRUE(isServiceExportSuccessful);

    waitForInterfacesAdded(dbusSignalHandler, dbusStubAdapterArray.size(), 0);

    ASSERT_TRUE(CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(managerDBusStubAdapter->getAddress()));
    managerDBusStubAdapter.reset();

    const bool wasServiceDeregistrationSuccessful = std::all_of(
                    dbusStubAdapterArray.begin(),
                    dbusStubAdapterArray.end(),
                    [](const std::shared_ptr<TestDBusStubAdapter>& dbusStubAdapter) {
                        return !CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterManagedService(
                                        dbusStubAdapter->getAddress());
                    });
    ASSERT_TRUE(wasServiceDeregistrationSuccessful);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
