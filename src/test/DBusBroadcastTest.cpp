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

class SelectiveBroadcastSender: public commonapi::tests::TestInterfaceStubDefault {
public:

    SelectiveBroadcastSender():
        acceptSubs(true),
        sentBroadcasts(0) {

    }

    void startSending() {
        sentBroadcasts = 0;
        selectiveBroadcastSender = std::thread(&SelectiveBroadcastSender::send, this);
        selectiveBroadcastSender.detach();
    }

    void send() {
        sentBroadcasts++;
        fireTestSelectiveBroadcastSelective();
    }

    void onTestSelectiveBroadcastSelectiveSubscriptionChanged(
            const std::shared_ptr<CommonAPI::ClientId> clientId,
            const CommonAPI::SelectiveBroadcastSubscriptionEvent event) {

        if (event == CommonAPI::SelectiveBroadcastSubscriptionEvent::SUBSCRIBED)
            lastSubscribedClient = clientId;
    }

    bool onTestSelectiveBroadcastSelectiveSubscriptionRequested(
                    const std::shared_ptr<CommonAPI::ClientId> clientId) {
        return acceptSubs;
    }

    void sendToLastSubscribedClient()
    {
        sentBroadcasts++;
        std::shared_ptr<CommonAPI::ClientIdList> receivers = std::make_shared<CommonAPI::ClientIdList>();
        receivers->insert(lastSubscribedClient);

        fireTestSelectiveBroadcastSelective(receivers);
    }


    int getNumberOfSubscribedClients() {
        return getSubscribersForTestSelectiveBroadcastSelective()->size();

    }

    bool acceptSubs;

private:
    std::thread selectiveBroadcastSender;
    int sentBroadcasts;

    std::shared_ptr<CommonAPI::ClientId> lastSubscribedClient;
};



class DBusBroadcastTest: public ::testing::Test {
protected:
   virtual void SetUp() {
       runtime_ = CommonAPI::Runtime::load();
       ASSERT_TRUE((bool)runtime_);
       CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
       ASSERT_TRUE(dbusRuntime != NULL);

       proxyFactory_ = runtime_->createFactory();
       ASSERT_TRUE((bool)proxyFactory_);

       proxyFactory2_ = runtime_->createFactory();
       ASSERT_TRUE((bool)proxyFactory2_);

       stubFactory_ = runtime_->createFactory();
       ASSERT_TRUE((bool)stubFactory_);

       servicePublisher_ = runtime_->getServicePublisher();
       ASSERT_TRUE((bool)servicePublisher_);

       selectiveBroadcastArrivedAtProxyFromSameFactory1 = 0;
       selectiveBroadcastArrivedAtProxyFromSameFactory2 = 0;
       selectiveBroadcastArrivedAtProxyFromOtherFactory = 0;
   }

   virtual void TearDown() {
       servicePublisher_->unregisterService(serviceAddress_);
   }

   std::shared_ptr<CommonAPI::Runtime> runtime_;
   std::shared_ptr<CommonAPI::Factory> proxyFactory_;
   std::shared_ptr<CommonAPI::Factory> proxyFactory2_;
   std::shared_ptr<CommonAPI::Factory> stubFactory_;
   std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;

   static const std::string serviceAddress_;
   static const std::string nonstandardAddress_;

   int selectiveBroadcastArrivedAtProxyFromSameFactory1;
   int selectiveBroadcastArrivedAtProxyFromSameFactory2;
   int selectiveBroadcastArrivedAtProxyFromOtherFactory;
public:
   void selectiveBroadcastCallbackForProxyFromSameFactory1() {
       selectiveBroadcastArrivedAtProxyFromSameFactory1++;
   }

   void selectiveBroadcastCallbackForProxyFromSameFactory2() {
       selectiveBroadcastArrivedAtProxyFromSameFactory2++;
   }

   void selectiveBroadcastCallbackForProxyFromOtherFactory() {
       selectiveBroadcastArrivedAtProxyFromOtherFactory++;
   }
};

const std::string DBusBroadcastTest::serviceAddress_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
const std::string DBusBroadcastTest::nonstandardAddress_ = "local:non.standard.ServiceName:non.standard.participand.ID";

TEST_F(DBusBroadcastTest, ProxysCanHandleBroadcast) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);
}

TEST_F(DBusBroadcastTest, ProxysCanUnsubscribeFromBroadcastAndSubscribeAgain) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    auto broadcastSubscription = broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription);

    callbackArrived = false;

    auto broadcastSubscription2 = broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainInALoop) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    for(unsigned int i=0; i<10; i++) {
        bool callbackArrived = false;

        auto broadcastSubscription = broadcastEvent.subscribe([&,i](uint32_t intParam, std::string stringParam) {
                EXPECT_EQ(intParam, i);
                callbackArrived = true;
        });

        stub->fireTestPredefinedTypeBroadcastEvent(i, "xyz");

        for(unsigned int j=0; j<100 && !callbackArrived; j++) {
            usleep(10000);
        }

        ASSERT_TRUE(callbackArrived);

        broadcastEvent.unsubscribe(broadcastSubscription);
    }
}

TEST_F(DBusBroadcastTest, ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainWithOtherProxy) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    auto broadcastSubscription = broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription);

    auto proxy2 = proxyFactory2_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    callbackArrived = false;

    auto broadcastSubscription2 = broadcastEvent2.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, ProxysCanCancelSubscriptionAndSubscribeAgainWithOtherProxy) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::CANCEL;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    auto proxy2 = proxyFactory2_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    callbackArrived = false;

    auto broadcastSubscription2 = broadcastEvent2.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent2.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainWhileOtherProxyIsStillSubscribed) {
    // register service
    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    // build 2 proxies from same factory
    auto proxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    auto proxy2 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy2->getTestPredefinedTypeBroadcastEvent();

    bool callback1Arrived = false;
    bool callback2Arrived = false;

    // subscribe for each proxy's broadcast event
    auto broadcastSubscription = broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            callback1Arrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    auto broadcastSubscription2 = broadcastEvent2.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            callback2Arrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    // fire broadcast and wait for results
    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !(callback1Arrived && callback2Arrived); i++) {
        usleep(10000);
    }

    const bool callbackOnBothSubscriptionsArrived = callback1Arrived && callback2Arrived;

    EXPECT_TRUE(callbackOnBothSubscriptionsArrived);

    // unsubscribe from one proxy's broadcast
    broadcastEvent.unsubscribe(broadcastSubscription);

    // fire broadcast again
    callback1Arrived = false;
    callback2Arrived = false;

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100; i++) {
        usleep(10000);
    }

    const bool onlyCallback2Arrived = !callback1Arrived && callback2Arrived;

    EXPECT_TRUE(onlyCallback2Arrived);

    // subscribe first proxy again
    broadcastSubscription = broadcastEvent.subscribeCancellableListener([&](uint32_t intParam, std::string stringParam) -> CommonAPI::SubscriptionStatus {
            callback1Arrived = true;
            return CommonAPI::SubscriptionStatus::RETAIN;
    });

    // fire broadcast another time
    callback1Arrived = false;
    callback2Arrived = false;

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !(callback1Arrived && callback2Arrived); i++) {
        usleep(10000);
    }

    const bool callbackOnBothSubscriptionsArrivedAgain = callback1Arrived && callback2Arrived;

    EXPECT_TRUE(callbackOnBothSubscriptionsArrivedAgain);

    broadcastEvent.unsubscribe(broadcastSubscription);
    broadcastEvent2.unsubscribe(broadcastSubscription2);
}



TEST_F(DBusBroadcastTest, ProxysCanSubscribeForSelectiveBroadcast)
{
    auto proxyFromSameFactory1 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)proxyFromSameFactory1);
    auto proxyFromSameFactory2 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)proxyFromSameFactory2);
    auto proxyFromOtherFactory = proxyFactory2_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)proxyFromOtherFactory);

    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for (unsigned int i = 0; !proxyFromSameFactory1->isAvailable() && i < 200; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(proxyFromSameFactory1->isAvailable());

    auto subscriptionResult1 = proxyFromSameFactory1->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory1, this));

    usleep(20000);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);

    stub->send();

    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 1);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 0);


    auto subscriptionResult2 = proxyFromSameFactory2->getTestSelectiveBroadcastSelectiveEvent().subscribe(std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory2, this));
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1); // should still be one because these were created by the same factory thus using the same connection

    stub->send();
    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 2);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 1);

    proxyFromOtherFactory->getTestSelectiveBroadcastSelectiveEvent().subscribe(std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromOtherFactory, this));
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 2); // should still be two because proxyFromSameFactory1_ is still subscribed

    proxyFromSameFactory2->getTestSelectiveBroadcastSelectiveEvent().unsubscribe(subscriptionResult2);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 2); // should still be two because proxyFromSameFactory1_ is still subscribed

    stub->send();
    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 3);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 1);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromOtherFactory, 1);

    // now only the last subscribed client (which is the one from the other factory) should receive the signal
    stub->sendToLastSubscribedClient();
    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 3);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 1);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromOtherFactory, 2);

    proxyFromSameFactory1->getTestSelectiveBroadcastSelectiveEvent().unsubscribe(subscriptionResult1);
    EXPECT_EQ(stub->getNumberOfSubscribedClients(), 1);
}

TEST_F(DBusBroadcastTest, ProxysCanBeRejectedForSelectiveBroadcast) {
    auto proxyFromSameFactory1 = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)proxyFromSameFactory1);
    auto proxyFromOtherFactory = proxyFactory2_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)proxyFromOtherFactory);


    auto stub = std::make_shared<SelectiveBroadcastSender>();

    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for (unsigned int i = 0; !proxyFromSameFactory1->isAvailable() && i < 200; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(proxyFromSameFactory1->isAvailable());

    bool subbed = false;

    proxyFromSameFactory1->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory1, this),
                    subbed);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    ASSERT_TRUE(subbed);

    stub->acceptSubs = false;

    proxyFromOtherFactory->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromOtherFactory, this),
                    subbed);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    ASSERT_FALSE(subbed);

    stub->send();

    usleep(20000);
    ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 1);
    ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromOtherFactory, 0);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
