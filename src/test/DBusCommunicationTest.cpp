/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <tuple>
#include <type_traits>

#include <dbus/dbus.h>

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"


class DBusCommunicationTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);

        proxyFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool)proxyFactory_);
        stubFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool)stubFactory_);

        servicePublisher_ = runtime_->getServicePublisher();
        ASSERT_TRUE((bool)servicePublisher_);
    }

    virtual void TearDown() {
        usleep(30000);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::Factory> proxyFactory_;
    std::shared_ptr<CommonAPI::Factory> stubFactory_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;

    static const std::string serviceAddress_;
    static const std::string serviceAddress2_;
    static const std::string serviceAddress3_;
    static const std::string serviceAddress4_;
    static const std::string nonstandardAddress_;
};

const std::string DBusCommunicationTest::serviceAddress_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
const std::string DBusCommunicationTest::serviceAddress2_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService2";
const std::string DBusCommunicationTest::serviceAddress3_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService3";
const std::string DBusCommunicationTest::serviceAddress4_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService4";
const std::string DBusCommunicationTest::nonstandardAddress_ = "local:non.standard.ServiceName:non.standard.participand.ID";


TEST_F(DBusCommunicationTest, RemoteMethodCallSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Ciao ;)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);

    servicePublisher_->unregisterService(serviceAddress_);
}


TEST_F(DBusCommunicationTest, SameStubCanBeRegisteredSeveralTimes) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    auto defaultTestProxy2 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress2_);
    auto defaultTestProxy3 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress3_);
    ASSERT_TRUE((bool)defaultTestProxy);
    ASSERT_TRUE((bool)defaultTestProxy2);
    ASSERT_TRUE((bool)defaultTestProxy3);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    bool serviceRegistered2 = servicePublisher_->registerService(stub, serviceAddress2_, stubFactory_);
    bool serviceRegistered3 = servicePublisher_->registerService(stub, serviceAddress3_, stubFactory_);
    for (unsigned int i = 0; (!serviceRegistered || !serviceRegistered2 || !serviceRegistered3) && i < 100; ++i) {
        if (!serviceRegistered) {
            serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        }
        if (!serviceRegistered2) {
            serviceRegistered2 = servicePublisher_->registerService(stub, serviceAddress2_, stubFactory_);
        }
        if (!serviceRegistered3) {
            serviceRegistered3 = servicePublisher_->registerService(stub, serviceAddress3_, stubFactory_);
        }
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);
    ASSERT_TRUE(serviceRegistered2);
    ASSERT_TRUE(serviceRegistered3);

    for(unsigned int i = 0; (!defaultTestProxy->isAvailable() || !defaultTestProxy2->isAvailable() || !defaultTestProxy3->isAvailable()) && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());
    ASSERT_TRUE(defaultTestProxy2->isAvailable());
    ASSERT_TRUE(defaultTestProxy3->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Ciao ;)";
    CommonAPI::CallStatus stat, stat2, stat3;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);
    defaultTestProxy2->testVoidPredefinedTypeMethod(v1, v2, stat2);
    defaultTestProxy3->testVoidPredefinedTypeMethod(v1, v2, stat3);

    EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(stat2, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(stat3, CommonAPI::CallStatus::SUCCESS);

    servicePublisher_->unregisterService(serviceAddress_);
}


TEST_F(DBusCommunicationTest, RemoteMethodCallWithNonstandardAddressSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(nonstandardAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = servicePublisher_->registerService(stub, nonstandardAddress_, stubFactory_);
    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, nonstandardAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    servicePublisher_->unregisterService(nonstandardAddress_);
}


TEST_F(DBusCommunicationTest, RemoteMethodCallHeavyLoad) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress4_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress4_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress4_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for (unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Ciao ;)";
    CommonAPI::CallStatus stat;

    for (uint32_t i = 0; i < 1000; i++) {
        defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);
        EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    }

    servicePublisher_->unregisterService(serviceAddress4_);
}


//XXX This test case requires CommonAPI::DBus::DBusConnection::suspendDispatching and ...::resumeDispatching to be public!

//static const std::string commonApiAddress = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
//static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
//static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
//static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTestService";

//TEST_F(DBusCommunicationTest, AsyncCallsAreQueuedCorrectly) {
//    auto proxyDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
//    ASSERT_TRUE(proxyDBusConnection->connect());
//
//    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
//
//    bool serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
//    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
//        serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
//        usleep(10000);
//    }
//    ASSERT_TRUE(serviceRegistered);
//
//    auto defaultTestProxy = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
//                            commonApiAddress,
//                            interfaceName,
//                            busName,
//                            objectPath,
//                            proxyDBusConnection);
//
//    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
//        usleep(10000);
//    }
//    ASSERT_TRUE(defaultTestProxy->isAvailable());
//
//    auto val1 = commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK;
//    commonapi::tests::DerivedTypeCollection::TestMap val2;
//    CommonAPI::CallStatus status;
//    unsigned int numCalled = 0;
//    const unsigned int maxNumCalled = 1000;
//    for(unsigned int i = 0; i < maxNumCalled/2; ++i) {
//        defaultTestProxy->testVoidDerivedTypeMethodAsync(val1, val2,
//                [&] (CommonAPI::CallStatus stat) {
//                    if(stat == CommonAPI::CallStatus::SUCCESS) {
//                        numCalled++;
//                    }
//                }
//        );
//    }
//
//    proxyDBusConnection->suspendDispatching();
//
//    for(unsigned int i = maxNumCalled/2; i < maxNumCalled; ++i) {
//        defaultTestProxy->testVoidDerivedTypeMethodAsync(val1, val2,
//                [&] (CommonAPI::CallStatus stat) {
//                    if(stat == CommonAPI::CallStatus::SUCCESS) {
//                        numCalled++;
//                    }
//                }
//        );
//    }
//    sleep(2);
//
//    proxyDBusConnection->resumeDispatching();
//
//    sleep(2);
//
//    ASSERT_EQ(maxNumCalled, numCalled);
//
//    numCalled = 0;
//
//    defaultTestProxy->getTestPredefinedTypeBroadcastEvent().subscribe(
//            [&] (uint32_t, std::string) {
//                numCalled++;
//            }
//    );
//
//    proxyDBusConnection->suspendDispatching();
//
//    for(unsigned int i = 0; i < maxNumCalled; ++i) {
//        stub->fireTestPredefinedTypeBroadcastEvent(0, "Nonething");
//    }
//
//    sleep(2);
//    proxyDBusConnection->resumeDispatching();
//    sleep(2);
//
//    ASSERT_EQ(maxNumCalled, numCalled);
//}



class DBusLowLevelCommunicationTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);

        proxyFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool)proxyFactory_);

        dummy = std::shared_ptr<CommonAPI::DBus::DBusFactory>(NULL);
    }

    virtual void TearDown() {
        usleep(30000);
    }

    std::shared_ptr<CommonAPI::DBus::DBusStubAdapter> createDBusStubAdapter(std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection,
                                                                            const std::string& commonApiAddress) {
        std::string interfaceName;
        std::string connectionName;
        std::string objectPath;
        CommonAPI::DBus::DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);

        std::shared_ptr<CommonAPI::DBus::DBusStubAdapter> dbusStubAdapter;
        std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

        dbusStubAdapter = std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(dummy, commonApiAddress, interfaceName, connectionName, objectPath, dbusConnection, stub);
        dbusStubAdapter->init();

        std::shared_ptr<CommonAPI::DBus::DBusObjectManagerStub> rootDBusObjectManagerStub = dbusConnection->getDBusObjectManager()->getRootDBusObjectManagerStub();

        const auto dbusObjectManager = dbusConnection->getDBusObjectManager();
        const bool isDBusObjectRegistrationSuccessful = dbusObjectManager->registerDBusStubAdapter(dbusStubAdapter);

        const bool isServiceExportSuccessful = rootDBusObjectManagerStub->exportManagedDBusStubAdapter(dbusStubAdapter);

        return dbusStubAdapter;
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::Factory> proxyFactory_;

    std::shared_ptr<CommonAPI::DBus::DBusFactory> dummy;

    static const std::string lowLevelAddress_;
    static const std::string lowLevelConnectionName_;
    static const std::string lowLevelAddress2_;
    static const std::string lowLevelAddress3_;
    static const std::string lowLevelAddress4_;
};

const std::string DBusLowLevelCommunicationTest::lowLevelAddress_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyLowLevelService";
const std::string DBusLowLevelCommunicationTest::lowLevelConnectionName_ = "CommonAPI.DBus.tests.DBusProxyLowLevelService";
const std::string DBusLowLevelCommunicationTest::lowLevelAddress2_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyLowLevelService2";
const std::string DBusLowLevelCommunicationTest::lowLevelAddress3_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyLowLevelService3";
const std::string DBusLowLevelCommunicationTest::lowLevelAddress4_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyLowLevelService4";

::DBusHandlerResult onLibdbusObjectPathMessageThunk(::DBusConnection* libdbusConnection,
                                                    ::DBusMessage* libdbusMessage,
                                                    void* userData) {
   return ::DBusHandlerResult::DBUS_HANDLER_RESULT_HANDLED;
}

DBusObjectPathVTable libdbusObjectPathVTable = {
               NULL,
               &onLibdbusObjectPathMessageThunk
};


TEST_F(DBusLowLevelCommunicationTest, AgressiveNameClaimingOfServicesIsHandledCorrectly) {
    std::shared_ptr<CommonAPI::DBus::DBusConnection> connection1 = CommonAPI::DBus::DBusConnection::getSessionBus();
    std::shared_ptr<CommonAPI::DBus::DBusConnection> connection2 = CommonAPI::DBus::DBusConnection::getSessionBus();

    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(lowLevelAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    uint32_t counter = 0;
    CommonAPI::AvailabilityStatus status;

    CommonAPI::ProxyStatusEvent& proxyStatusEvent = defaultTestProxy->getProxyStatusEvent();
    proxyStatusEvent.subscribe([&counter, &status](const CommonAPI::AvailabilityStatus& stat) {
        ++counter;
        status = stat;
    });

    sleep(1);

    EXPECT_EQ(1, counter);
    EXPECT_EQ(CommonAPI::AvailabilityStatus::NOT_AVAILABLE, status);

    //Set up low level connections
    ::DBusConnection* libdbusConnection1 = dbus_bus_get_private(DBUS_BUS_SESSION, NULL);
    ::DBusConnection* libdbusConnection2 = dbus_bus_get_private(DBUS_BUS_SESSION, NULL);

    ASSERT_TRUE(libdbusConnection1);
    ASSERT_TRUE(libdbusConnection2);

    dbus_connection_set_exit_on_disconnect(libdbusConnection1, false);
    dbus_connection_set_exit_on_disconnect(libdbusConnection2, false);

    bool endDispatch = false;
    std::promise<bool> ended;
    std::future<bool> hasEnded = ended.get_future();

    std::thread([&]() {
            dbus_bool_t libdbusSuccess = true;
            while (!endDispatch && libdbusSuccess) {
                libdbusSuccess = dbus_connection_read_write_dispatch(libdbusConnection1, 10);
                libdbusSuccess &= dbus_connection_read_write_dispatch(libdbusConnection2, 10);
            }
            ended.set_value(true);
    }).detach();

    //Test first connect
    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection1 = std::make_shared<CommonAPI::DBus::DBusConnection>(libdbusConnection1);
    ASSERT_TRUE(dbusConnection1->isConnected());
    std::shared_ptr<CommonAPI::DBus::DBusStubAdapter> adapter1 = createDBusStubAdapter(dbusConnection1, lowLevelAddress_);

    int libdbusStatus = dbus_bus_request_name(libdbusConnection1,
                    lowLevelConnectionName_.c_str(),
                    DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_REPLACE_EXISTING,
                    NULL);

    dbus_connection_try_register_object_path(libdbusConnection1,
                    "/",
                    &libdbusObjectPathVTable,
                    NULL,
                    NULL);

    sleep(1);

    EXPECT_EQ(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER, libdbusStatus);
    EXPECT_EQ(2, counter);
    EXPECT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, status);

    //Test second connect
    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection2 = std::make_shared<CommonAPI::DBus::DBusConnection>(libdbusConnection2);
    ASSERT_TRUE(dbusConnection2->isConnected());
    std::shared_ptr<CommonAPI::DBus::DBusStubAdapter> adapter2 = createDBusStubAdapter(dbusConnection2, lowLevelAddress_);

    libdbusStatus = dbus_bus_request_name(libdbusConnection2,
                    lowLevelConnectionName_.c_str(),
                    DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_REPLACE_EXISTING,
                    NULL);

    dbus_connection_try_register_object_path(libdbusConnection2,
                    "/",
                    &libdbusObjectPathVTable,
                    NULL,
                    NULL);

    sleep(1);

    EXPECT_EQ(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER, libdbusStatus);

    //4 Because a short phase of non-availability will be inbetween
    EXPECT_EQ(4, counter);
    EXPECT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, status);

    //Close connections
    endDispatch = true;
    ASSERT_TRUE(hasEnded.get());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
