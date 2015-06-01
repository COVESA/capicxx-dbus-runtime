// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

#include <CommonAPI/CommonAPI.hpp>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/DBus/DBusConnection.hpp>
#include <CommonAPI/DBus/DBusProxy.hpp>

#include "commonapi/tests/PredefinedTypeCollection.hpp"
#include "commonapi/tests/DerivedTypeCollection.hpp"
#include "v1_0/commonapi/tests/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/tests/TestInterfaceStubDefault.hpp"
#include "v1_0/commonapi/tests/TestInterfaceDBusStubAdapter.hpp"

#include "v1_0/commonapi/tests/TestInterfaceDBusProxy.hpp"

#define VERSION v1_0

class SelectiveBroadcastSender: public VERSION::commonapi::tests::TestInterfaceStubDefault {
public:

    SelectiveBroadcastSender():
        acceptSubs(true),
        sentBroadcasts(0) {

	}

	virtual ~SelectiveBroadcastSender() {

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
       runtime_ = CommonAPI::Runtime::get();
       ASSERT_TRUE((bool)runtime_);

	   serviceAddressObject_ = CommonAPI::Address(serviceAddress_);

       selectiveBroadcastArrivedAtProxyFromSameFactory1 = 0;
       selectiveBroadcastArrivedAtProxyFromSameFactory2 = 0;
       selectiveBroadcastArrivedAtProxyFromOtherFactory = 0;
   }

   virtual void TearDown() {
	   runtime_->unregisterService(serviceAddressObject_.getDomain(), serviceAddressInterface_, serviceAddressObject_.getInstance());
   }

   std::shared_ptr<CommonAPI::Runtime> runtime_;

   static const std::string serviceAddress_;
   CommonAPI::Address serviceAddressObject_;
   std::string serviceAddressInterface_;
   static const CommonAPI::ConnectionId_t connectionId_;

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
const CommonAPI::ConnectionId_t DBusBroadcastTest::connectionId_ = "connection";

TEST_F(DBusBroadcastTest, ProxysCanHandleBroadcast) {
    auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);



	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);
}

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanUnsubscribeFromBroadcastAndSubscribeAgain) {
	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);



	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

    auto broadcastSubscription = broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription);

    callbackArrived = false;

	auto broadcastSubscription2 = broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

	broadcastEvent.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainInALoop) {
	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
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

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainWithOtherProxy) {
	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

	auto broadcastSubscription = broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

    broadcastEvent.unsubscribe(broadcastSubscription);

	auto proxy2 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    callbackArrived = false;

	auto broadcastSubscription2 = broadcastEvent2.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

	broadcastEvent.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanCancelSubscriptionAndSubscribeAgainWithOtherProxy) {
	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);


	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    bool callbackArrived = false;

	broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 1);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(1, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

	auto proxy2 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    callbackArrived = false;

	auto broadcastSubscription2 = broadcastEvent2.subscribe([&](uint32_t intParam, std::string stringParam) {
            EXPECT_EQ(intParam, 2);
            callbackArrived = true;
    });

    stub->fireTestPredefinedTypeBroadcastEvent(2, "xyz");

    for(unsigned int i=0; i<100 && !callbackArrived; i++) {
        usleep(10000);
    }

    ASSERT_TRUE(callbackArrived);

	broadcastEvent2.unsubscribe(broadcastSubscription2);
}

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanUnsubscribeFromBroadcastAndSubscribeAgainWhileOtherProxyIsStillSubscribed) {
    // register service
	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    // build 2 proxies from same factory
	auto proxy = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
	auto proxy2 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent =
                    proxy->getTestPredefinedTypeBroadcastEvent();

    VERSION::commonapi::tests::TestInterfaceProxyDefault::TestPredefinedTypeBroadcastEvent& broadcastEvent2 =
                    proxy2->getTestPredefinedTypeBroadcastEvent();

    bool callback1Arrived = false;
    bool callback2Arrived = false;

    // subscribe for each proxy's broadcast event
    auto broadcastSubscription = broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            callback1Arrived = true;
    });

	auto broadcastSubscription2 = broadcastEvent2.subscribe([&](uint32_t intParam, std::string stringParam) {
            callback2Arrived = true;
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
	broadcastSubscription = broadcastEvent.subscribe([&](uint32_t intParam, std::string stringParam) {
            callback1Arrived = true;
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

TEST_F(DBusBroadcastTest, DISABLED_ProxysCanSubscribeForSelectiveBroadcast)
{
	auto proxyFromSameFactory1 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
    ASSERT_TRUE((bool)proxyFromSameFactory1);
	auto proxyFromSameFactory2 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
    ASSERT_TRUE((bool)proxyFromSameFactory2);
	auto proxyFromOtherFactory = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
    ASSERT_TRUE((bool)proxyFromOtherFactory);

	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
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
	auto proxyFromSameFactory1 = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
    ASSERT_TRUE((bool)proxyFromSameFactory1);
	auto proxyFromOtherFactory = runtime_->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance());
    ASSERT_TRUE((bool)proxyFromOtherFactory);


	auto stub = std::make_shared<SelectiveBroadcastSender>();
	serviceAddressInterface_ = stub->getStubAdapter()->getInterface();

	bool serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
    for (unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
		serviceRegistered = runtime_->registerService(serviceAddressObject_.getDomain(), serviceAddressObject_.getInstance(), stub, connectionId_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for (unsigned int i = 0; !proxyFromSameFactory1->isAvailable() && i < 200; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(proxyFromSameFactory1->isAvailable());

    bool subbed = false;

    proxyFromSameFactory1->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromSameFactory1, this));
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    //ASSERT_TRUE(subbed);

    stub->acceptSubs = false;

    proxyFromOtherFactory->getTestSelectiveBroadcastSelectiveEvent().subscribe(
                    std::bind(&DBusBroadcastTest::selectiveBroadcastCallbackForProxyFromOtherFactory, this));
    ASSERT_EQ(stub->getNumberOfSubscribedClients(), 1);
    //ASSERT_FALSE(subbed);

    stub->send();

    usleep(20000);
    ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromSameFactory1, 1);
	ASSERT_EQ(selectiveBroadcastArrivedAtProxyFromOtherFactory, 0);
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
