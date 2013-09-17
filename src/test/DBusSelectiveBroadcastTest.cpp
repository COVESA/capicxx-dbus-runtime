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



class DBusSelectiveBroadcastTest: public ::testing::Test {
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
   CommonAPI::SubscriptionStatus selectiveBroadcastCallbackForProxyFromSameFactory1() {
       selectiveBroadcastArrivedAtProxyFromSameFactory1++;
       return CommonAPI::SubscriptionStatus::RETAIN;
   }

   CommonAPI::SubscriptionStatus selectiveBroadcastCallbackForProxyFromSameFactory2() {
       selectiveBroadcastArrivedAtProxyFromSameFactory2++;
       return CommonAPI::SubscriptionStatus::RETAIN;
   }

   CommonAPI::SubscriptionStatus selectiveBroadcastCallbackForProxyFromOtherFactory() {
       selectiveBroadcastArrivedAtProxyFromOtherFactory++;
       return CommonAPI::SubscriptionStatus::RETAIN;
   }
};

const std::string DBusSelectiveBroadcastTest::serviceAddress_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
const std::string DBusSelectiveBroadcastTest::nonstandardAddress_ = "local:non.standard.ServiceName:non.standard.participand.ID";


TEST_F(DBusSelectiveBroadcastTest, ProxysCanSubscribe)
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
                    std::bind(&DBusSelectiveBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory1, this));

    usleep(20000);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);

    stub->send();

    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 1);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 0);


    auto subscriptionResult2 = proxyFromSameFactory2->getTestSelectiveBroadcastSelectiveEvent().subscribe(std::bind(&DBusSelectiveBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory2, this));
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1); // should still be one because these were created by the same factory thus using the same connection

    stub->send();
    usleep(200000);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 2);
    EXPECT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory2, 1);

    auto subscriptionResult3 = proxyFromOtherFactory->getTestSelectiveBroadcastSelectiveEvent().subscribe(std::bind(&DBusSelectiveBroadcastTest::selectiveBroadcastCallbackForProxyFromOtherFactory, this));
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

TEST_F(DBusSelectiveBroadcastTest, ProxysCanBeRejected) {
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

    auto subscriptionResult1 = proxyFromSameFactory1->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusSelectiveBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory1, this),
                    subbed);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    ASSERT_TRUE(subbed);

    stub->acceptSubs = false;

    auto subscriptionResult2 = proxyFromOtherFactory->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusSelectiveBroadcastTest::selectiveBroadcastCallbackForProxyFromOtherFactory, this),
                    subbed);
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    ASSERT_FALSE(subbed);

    stub->send();

    usleep(20000);
    ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 1);
    ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromOtherFactory, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
