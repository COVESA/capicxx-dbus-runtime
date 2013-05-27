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
#include <glib.h>

#include <CommonAPI/types.h>
#include <CommonAPI/AttributeExtension.h>
#include <CommonAPI/Runtime.h>

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "DBusTestUtils.h"
#include "DemoMainLoop.h"

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"


const std::string testAddress1 = "local:my.first.test:commonapi.address.one";
const std::string testAddress2 = "local:my.second.test:commonapi.address.two";
const std::string testAddress3 = "local:my.third.test:commonapi.address.three";
const std::string testAddress4 = "local:my.fourth.test:commonapi.address.four";
const std::string testAddress5 = "local:my.fifth.test:commonapi.address.five";
const std::string testAddress6 = "local:my.sixth.test:commonapi.address.six";
const std::string testAddress7 = "local:my.seventh.test:commonapi.address.seven";
const std::string testAddress8 = "local:my.eigth.test:commonapi.address.eight";

//####################################################################################################################

class DBusBasicMainLoopTest: public ::testing::Test {
protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


TEST_F(DBusBasicMainLoopTest, MainloopContextCanBeCreated) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool) runtime);

    std::shared_ptr<CommonAPI::MainLoopContext> context = runtime->getNewMainLoopContext();
    ASSERT_TRUE((bool) context);
}


TEST_F(DBusBasicMainLoopTest, SeveralMainloopContextsCanBeCreated) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::MainLoopContext> context1 = runtime->getNewMainLoopContext();
    ASSERT_TRUE((bool) context1);
    std::shared_ptr<CommonAPI::MainLoopContext> context2 = runtime->getNewMainLoopContext();
    ASSERT_TRUE((bool) context2);
    std::shared_ptr<CommonAPI::MainLoopContext> context3 = runtime->getNewMainLoopContext();
    ASSERT_TRUE((bool) context3);

    ASSERT_NE(context1, context2);
    ASSERT_NE(context1, context3);
    ASSERT_NE(context2, context3);
}

struct TestSource: public CommonAPI::DispatchSource {
    TestSource(const std::string value, std::string& result): value_(value), result_(result) {}

    bool prepare(int64_t& timeout) {
        return true;
    }
    bool check() {
        return true;
    }
    bool dispatch() {
        result_.append(value_);
        return false;
    }

 private:
    std::string value_;
    std::string& result_;
};

TEST_F(DBusBasicMainLoopTest, PrioritiesAreHandledCorrectlyInDemoMainloop) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool) runtime);

    std::shared_ptr<CommonAPI::MainLoopContext> context = runtime->getNewMainLoopContext();
    ASSERT_TRUE((bool) context);

    auto mainLoop = new CommonAPI::MainLoop(context);

    std::string result = "";

    TestSource* testSource1Default = new TestSource("A", result);
    TestSource* testSource2Default = new TestSource("B", result);
    TestSource* testSource1High = new TestSource("C", result);
    TestSource* testSource1Low = new TestSource("D", result);
    TestSource* testSource1VeryHigh = new TestSource("E", result);
    context->registerDispatchSource(testSource1Default);
    context->registerDispatchSource(testSource2Default, CommonAPI::DispatchPriority::DEFAULT);
    context->registerDispatchSource(testSource1High, CommonAPI::DispatchPriority::HIGH);
    context->registerDispatchSource(testSource1Low, CommonAPI::DispatchPriority::LOW);
    context->registerDispatchSource(testSource1VeryHigh, CommonAPI::DispatchPriority::VERY_HIGH);

    mainLoop->doSingleIteration(CommonAPI::TIMEOUT_INFINITE);

    std::string reference("ECABD");
    ASSERT_EQ(reference, result);
}


//####################################################################################################################

class DBusMainLoopTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool) runtime_);

        context_ = runtime_->getNewMainLoopContext();
        ASSERT_TRUE((bool) context_);
        mainLoop_ = new CommonAPI::MainLoop(context_);

        mainloopFactory_ = runtime_->createFactory(context_);
        ASSERT_TRUE((bool) mainloopFactory_);
        standardFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool) standardFactory_);
    }

    virtual void TearDown() {
        delete mainLoop_;
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::MainLoopContext> context_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactory_;
    std::shared_ptr<CommonAPI::Factory> standardFactory_;
    CommonAPI::MainLoop* mainLoop_;
};


TEST_F(DBusMainLoopTest, ServiceInDemoMainloopCanBeAddressed) {
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(mainloopFactory_->registerService(stub, testAddress1));

    auto proxy = standardFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress1);
    ASSERT_TRUE((bool) proxy);

    while (!proxy->isAvailable()) {
        mainLoop_->doSingleIteration(50000);
    }

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        mainLoop_->stop();
                    }
    );

    mainLoop_->run();

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    mainloopFactory_->unregisterService(testAddress1);
}


TEST_F(DBusMainLoopTest, ProxyInDemoMainloopCanCallMethods) {
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(standardFactory_->registerService(stub, testAddress2));

    usleep(500000);

    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress2);
    ASSERT_TRUE((bool) proxy);

    while (!proxy->isAvailable()) {
        mainLoop_->doSingleIteration();
        usleep(50000);
    }

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        mainLoop_->stop();
                    }
    );

    mainLoop_->run();

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    standardFactory_->unregisterService(testAddress2);
}

TEST_F(DBusMainLoopTest, ProxyAndServiceInSameDemoMainloopCanCommunicate) {
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(mainloopFactory_->registerService(stub, testAddress4));

    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress4);
    ASSERT_TRUE((bool) proxy);

    while (!proxy->isAvailable()) {
        mainLoop_->doSingleIteration();
        usleep(50000);
    }

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";
    bool running = true;

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        mainLoop_->stop();
                    }
    );

    mainLoop_->run();

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    mainloopFactory_->unregisterService(testAddress4);
}


class BigDataTestStub: public commonapi::tests::TestInterfaceStubDefault {
    void testDerivedTypeMethod(
            commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2InValue,
            commonapi::tests::DerivedTypeCollection::TestMap testMapInValue,
            commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue,
            commonapi::tests::DerivedTypeCollection::TestMap& testMapOutValue) {
        testEnumExtended2OutValue = testEnumExtended2InValue;
        testMapOutValue = testMapInValue;
    }
};


TEST_F(DBusMainLoopTest, ProxyAndServiceInSameDemoMainloopCanHandleBigData) {
    std::shared_ptr<BigDataTestStub> stub = std::make_shared<
                    BigDataTestStub>();
    ASSERT_TRUE(mainloopFactory_->registerService(stub, testAddress8));

    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    while (!proxy->isAvailable()) {
        mainLoop_->doSingleIteration();
        usleep(50000);
    }

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";
    bool running = true;

    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2InValue =
            commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK;
    commonapi::tests::DerivedTypeCollection::TestMap testMapInValue;

    // Estimated amount of data (differring padding at beginning/end of Map/Array etc. not taken into account):
    // 4 + 4 + 100 * (4 + (4 + 4 + 100 * (11 + 1 + 4)) + 4 ) = 161608
    for(uint32_t i = 0; i < 500; ++i) {
        commonapi::tests::DerivedTypeCollection::TestArrayTestStruct testArrayTestStruct;
        for(uint32_t j = 0; j < 100; ++j) {
            commonapi::tests::DerivedTypeCollection::TestStruct testStruct("Hai all (:", j);
            testArrayTestStruct.push_back(testStruct);
        }
        testMapInValue.insert( {i, testArrayTestStruct} );
    }

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testDerivedTypeMethodAsync(
            testEnumExtended2InValue,
            testMapInValue,
            [&] (const CommonAPI::CallStatus& status,
                    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2OutValue,
                    commonapi::tests::DerivedTypeCollection::TestMap testMapOutValue) {
                EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                EXPECT_EQ(testEnumExtended2InValue, testEnumExtended2OutValue);
                EXPECT_EQ(testMapInValue.size(), testMapOutValue.size());
                mainLoop_->stop();
            }
    );

    mainLoop_->run();

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    mainloopFactory_->unregisterService(testAddress8);
}

TEST_F(DBusMainLoopTest, DemoMainloopClientsHandleNonavailableServices) {
    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress3);
    ASSERT_TRUE((bool) proxy);

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        //Will never be called, @see DBusProxyAsyncCallbackHandler
                        EXPECT_EQ(toString(CommonAPI::CallStatus::NOT_AVAILABLE), toString(status));
                    }
    );

    ASSERT_EQ(toString(CommonAPI::CallStatus::NOT_AVAILABLE), toString(futureStatus.get()));
}

//##################################################################################################

class GDispatchWrapper: public GSource {
 public:
    GDispatchWrapper(CommonAPI::DispatchSource* dispatchSource): dispatchSource_(dispatchSource) {}
    CommonAPI::DispatchSource* dispatchSource_;
};

gboolean dispatchPrepare(GSource* source, gint* timeout) {
    int64_t eventTimeout;
    return static_cast<GDispatchWrapper*>(source)->dispatchSource_->prepare(eventTimeout);
}

gboolean dispatchCheck(GSource* source) {
    return static_cast<GDispatchWrapper*>(source)->dispatchSource_->check();
}

gboolean dispatchExecute(GSource* source, GSourceFunc callback, gpointer userData) {
    static_cast<GDispatchWrapper*>(source)->dispatchSource_->dispatch();
    return true;
}

gboolean gWatchDispatcher(GIOChannel *source, GIOCondition condition, gpointer userData) {
    CommonAPI::Watch* watch = static_cast<CommonAPI::Watch*>(userData);
    watch->dispatch(condition);
    return true;
}

gboolean gTimeoutDispatcher(void* userData) {
    return static_cast<CommonAPI::DispatchSource*>(userData)->dispatch();
}


static GSourceFuncs standardGLibSourceCallbackFuncs = {
        dispatchPrepare,
        dispatchCheck,
        dispatchExecute,
        NULL
};


class DBusInGLibMainLoopTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        running_ = true;

        std::shared_ptr<CommonAPI::Runtime> runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool) runtime_);

        context_ = runtime_->getNewMainLoopContext();
        ASSERT_TRUE((bool) context_);

        doSubscriptions();

        mainloopFactory_ = runtime_->createFactory(context_);
        ASSERT_TRUE((bool) mainloopFactory_);
        standardFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool) standardFactory_);
    }

    virtual void TearDown() {
    }

    void doSubscriptions() {
        context_->subscribeForTimeouts(
                std::bind(&DBusInGLibMainLoopTest::timeoutAddedCallback, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&DBusInGLibMainLoopTest::timeoutRemovedCallback, this, std::placeholders::_1)
        );

        context_->subscribeForWatches(
                std::bind(&DBusInGLibMainLoopTest::watchAddedCallback, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&DBusInGLibMainLoopTest::watchRemovedCallback, this, std::placeholders::_1)
        );

        context_->subscribeForWakeupEvents(
                std::bind(&DBusInGLibMainLoopTest::wakeupMain, this)
        );
    }


 public:
    std::shared_ptr<CommonAPI::MainLoopContext> context_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactory_;
    std::shared_ptr<CommonAPI::Factory> standardFactory_;
    bool running_;
    static constexpr bool mayBlock_ = true;

    std::map<CommonAPI::DispatchSource*, GSource*> gSourceMappings;

    GIOChannel* dbusChannel_;

    void watchAddedCallback(CommonAPI::Watch* watch, const CommonAPI::DispatchPriority dispatchPriority) {
        const pollfd& fileDesc = watch->getAssociatedFileDescriptor();
        dbusChannel_ = g_io_channel_unix_new(fileDesc.fd);

        GSource* gWatch = g_io_create_watch(dbusChannel_, static_cast<GIOCondition>(fileDesc.events));
        g_source_set_callback(gWatch, reinterpret_cast<GSourceFunc>(&gWatchDispatcher), watch, NULL);

        const auto& dependentSources = watch->getDependentDispatchSources();
        for (auto dependentSourceIterator = dependentSources.begin();
                dependentSourceIterator != dependentSources.end();
                dependentSourceIterator++) {
            GSource* gDispatchSource = g_source_new(&standardGLibSourceCallbackFuncs, sizeof(GDispatchWrapper));
            static_cast<GDispatchWrapper*>(gDispatchSource)->dispatchSource_ = *dependentSourceIterator;

            g_source_add_child_source(gWatch, gDispatchSource);

            gSourceMappings.insert( {*dependentSourceIterator, gDispatchSource} );
        }
        g_source_attach(gWatch, NULL);
    }

    void watchRemovedCallback(CommonAPI::Watch* watch) {
        g_source_remove_by_user_data(watch);

        if(dbusChannel_) {
            g_io_channel_unref(dbusChannel_);
            dbusChannel_ = NULL;
        }

        const auto& dependentSources = watch->getDependentDispatchSources();
        for (auto dependentSourceIterator = dependentSources.begin();
                dependentSourceIterator != dependentSources.end();
                dependentSourceIterator++) {
            GSource* gDispatchSource = g_source_new(&standardGLibSourceCallbackFuncs, sizeof(GDispatchWrapper));
            GSource* gSource = gSourceMappings.find(*dependentSourceIterator)->second;
            g_source_destroy(gSource);
            g_source_unref(gSource);
        }
    }

    void timeoutAddedCallback(CommonAPI::Timeout* commonApiTimeoutSource, const CommonAPI::DispatchPriority dispatchPriority) {
        GSource* gTimeoutSource = g_timeout_source_new(commonApiTimeoutSource->getTimeoutInterval());
        g_source_set_callback(gTimeoutSource, &gTimeoutDispatcher, commonApiTimeoutSource, NULL);
        g_source_attach(gTimeoutSource, NULL);
    }

    void timeoutRemovedCallback(CommonAPI::Timeout* timeout) {
        g_source_remove_by_user_data(timeout);
    }

    void wakeupMain() {
        g_main_context_wakeup(NULL);
    }
};


TEST_F(DBusInGLibMainLoopTest, ProxyInGLibMainloopCanCallMethods) {
    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress5);
    ASSERT_TRUE((bool) proxy);

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(standardFactory_->registerService(stub, testAddress5));

    while(!proxy->isAvailable()) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    uint32_t uint32Value = 24;
    std::string stringValue = "Hai :)";

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        running_ = false;
                    }
    );

    while(running_) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    standardFactory_->unregisterService(testAddress5);
}


TEST_F(DBusInGLibMainLoopTest, ServiceInGLibMainloopCanBeAddressed) {
    auto proxy = standardFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress6);
    ASSERT_TRUE((bool) proxy);

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(mainloopFactory_->registerService(stub, testAddress6));

    uint32_t uint32Value = 42;
    std::string stringValue = "Ciao (:";

    while(!proxy->isAvailable()) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        running_ = false;
                        //Wakeup needed as the service will be in a poll-block when the client
                        //call returns, and no other timeout is present to get him out of there.
                        g_main_context_wakeup(NULL);
                    }
    );

    while(running_) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    mainloopFactory_->unregisterService(testAddress6);
}


TEST_F(DBusInGLibMainLoopTest, ProxyAndServiceInSameGlibMainloopCanCommunicate) {
    auto proxy = mainloopFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress7);
    ASSERT_TRUE((bool) proxy);

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub = std::make_shared<
                    commonapi::tests::TestInterfaceStubDefault>();
    ASSERT_TRUE(mainloopFactory_->registerService(stub, testAddress7));

    uint32_t uint32Value = 42;
    std::string stringValue = "Ciao (:";

    while(!proxy->isAvailable()) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    std::future<CommonAPI::CallStatus> futureStatus = proxy->testVoidPredefinedTypeMethodAsync(
                    uint32Value,
                    stringValue,
                    [&] (const CommonAPI::CallStatus& status) {
                        EXPECT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(status));
                        running_ = false;
                        //Wakeup needed as the service will be in a poll-block when the client
                        //call returns, and no other timeout is present to get him out of there.
                        g_main_context_wakeup(NULL);
                    }
    );

    while(running_) {
        g_main_context_iteration(NULL, mayBlock_);
        usleep(50000);
    }

    ASSERT_EQ(toString(CommonAPI::CallStatus::SUCCESS), toString(futureStatus.get()));

    mainloopFactory_->unregisterService(testAddress7);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
