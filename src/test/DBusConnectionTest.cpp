/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxyAsyncCallbackHandler.h>

#include <gtest/gtest.h>
#include <dbus/dbus.h>

#include <cstring>


class DBusConnectionTest: public ::testing::Test {
 protected:
	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
};


//TEST_F(DBusConnectionTest, IsInitiallyDisconnected) {
//	ASSERT_FALSE(dbusConnection_->isConnected());
//}
//
//TEST_F(DBusConnectionTest, ConnectAndDisconnectWork) {
//	ASSERT_TRUE(dbusConnection_->connect());
//	ASSERT_TRUE(dbusConnection_->isConnected());
//
//	dbusConnection_->disconnect();
//	ASSERT_FALSE(dbusConnection_->isConnected());
//}
//
//TEST_F(DBusConnectionTest, ConnectionStatusEventWorks) {
//	ASSERT_EQ(connectionStatusEventCount_, 0);
//
//	auto connectionStatusSubscription = dbusConnection_->getConnectionStatusEvent().subscribe(std::bind(
//			&DBusConnectionTest::onConnectionStatusEvent,
//			this,
//			std::placeholders::_1));
//
//	ASSERT_FALSE(dbusConnection_->isConnected());
//	ASSERT_EQ(connectionStatusEventCount_, 0);
//
//	uint32_t expectedEventCount = 0;
//	while (expectedEventCount < 10) {
//		ASSERT_TRUE(dbusConnection_->connect());
//		ASSERT_TRUE(dbusConnection_->isConnected());
//		ASSERT_EQ(connectionStatusEventCount_, ++expectedEventCount);
//		ASSERT_EQ(connectionStatus_, common::api::AvailabilityStatus::AVAILABLE);
//
//		dbusConnection_->disconnect();
//		ASSERT_FALSE(dbusConnection_->isConnected());
//		ASSERT_EQ(connectionStatusEventCount_, ++expectedEventCount);
//		ASSERT_EQ(connectionStatus_, common::api::AvailabilityStatus::NOT_AVAILABLE);
//	}
//
//	dbusConnection_->getConnectionStatusEvent().unsubscribe(connectionStatusSubscription);
//	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);
//
//	ASSERT_TRUE(dbusConnection_->connect());
//	ASSERT_TRUE(dbusConnection_->isConnected());
//	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);
//
//	dbusConnection_->disconnect();
//	ASSERT_FALSE(dbusConnection_->isConnected());
//	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);
//}
//
//TEST_F(DBusConnectionTest, SendingAsyncDBusMessagesWorks) {
//	const char* busName = "common.api.dbus.test.TestInterfaceHandler";
//	const char* objectPath = "/common/api/dbus/test/TestObject";
//	const char* interfaceName = "common.api.dbus.test.TestInterface";
//	const char* methodName = "TestMethod";
//
//	auto interfaceHandlerDBusConnection = common::api::dbus::DBusConnection::getSessionBus();
//
//	ASSERT_TRUE(interfaceHandlerDBusConnection->connect());
//	ASSERT_TRUE(interfaceHandlerDBusConnection->requestServiceNameAndBlock(busName));
//
//	auto interfaceHandlerToken = interfaceHandlerDBusConnection->registerInterfaceHandler(
//			objectPath,
//			interfaceName,
//			std::bind(&DBusConnectionTest::onInterfaceHandlerDBusMessageReply,
//					  this,
//					  std::placeholders::_1,
//					  interfaceHandlerDBusConnection));
//
//
//	ASSERT_TRUE(dbusConnection_->connect());
//
//	for (uint32_t expectedDBusMessageCount = 1; expectedDBusMessageCount <= 10; expectedDBusMessageCount++) {
//		auto dbusMessageCall = common::api::dbus::DBusMessage::createMethodCall(
//				busName,
//				objectPath,
//				interfaceName,
//				methodName,
//				"si");
//		ASSERT_TRUE(dbusMessageCall);
//
//		common::api::dbus::DBusOutputMessageStream dbusOutputMessageStream(dbusMessageCall);
//		dbusOutputMessageStream << "This is a test async call"
//								<< expectedDBusMessageCount;
//		dbusOutputMessageStream.flush();
//
//		dbusConnection_->sendDBusMessageWithReplyAsync(
//				dbusMessageCall,
//				std::bind(&DBusConnectionTest::onDBusMessageHandler, this, std::placeholders::_1));
//
//		for (int i = 0; i < 10 && interfaceHandlerDBusMessageCount_ < expectedDBusMessageCount; i++)
//			interfaceHandlerDBusConnection->readWriteDispatch(100);
//
//		ASSERT_EQ(interfaceHandlerDBusMessageCount_, expectedDBusMessageCount);
//		ASSERT_DBUSMESSAGE_EQ(dbusMessageCall, interfaceHandlerDBusMessage_);
//
//		for (int i = 0; i < 10 && dbusMessageHandlerCount_ < expectedDBusMessageCount; i++)
//			dbusConnection_->readWriteDispatch(100);
//
//		ASSERT_EQ(dbusMessageHandlerCount_, expectedDBusMessageCount);
//		ASSERT_DBUSMESSAGE_EQ(dbusMessageHandlerDBusMessage_, interfaceHandlerDBusMessageReply_);
//	}
//
//	dbusConnection_->disconnect();
//
//
//	interfaceHandlerDBusConnection->unregisterInterfaceHandler(interfaceHandlerToken);
//
//	ASSERT_TRUE(interfaceHandlerDBusConnection->releaseServiceName(busName));
//	interfaceHandlerDBusConnection->disconnect();
//}


void dispatch(::DBusConnection* libdbusConnection) {
	dbus_bool_t success = TRUE;
	while(success) {
        success = dbus_connection_read_write_dispatch(libdbusConnection, 1);
    }
}

std::promise<bool> promise;
std::future<bool> future = promise.get_future();

void notifyThunk(DBusPendingCall*, void* data) {
	::DBusConnection* libdbusConnection = reinterpret_cast<DBusConnection*>(data);
	dbus_connection_close(libdbusConnection);
	dbus_connection_unref(libdbusConnection);
	promise.set_value(true);
}

TEST_F(DBusConnectionTest, LibdbusConnectionsMayCommitSuicide) {
	const ::DBusBusType libdbusType = ::DBusBusType::DBUS_BUS_SESSION;
	::DBusError libdbusError;
	dbus_error_init(&libdbusError);
	::DBusConnection* libdbusConnection = dbus_bus_get_private(libdbusType, &libdbusError);

	assert(libdbusConnection);
	dbus_connection_set_exit_on_disconnect(libdbusConnection, false);

	auto dispatchThread = std::thread(&dispatch, libdbusConnection);

	::DBusMessage* libdbusMessageCall = dbus_message_new_method_call(
			"org.freedesktop.DBus",
			"/org/freedesktop/DBus",
			"org.freedesktop.DBus",
            "ListNames");

	dbus_message_set_signature(libdbusMessageCall, "");

    DBusPendingCall* libdbusPendingCall;
    dbus_bool_t libdbusSuccess;

    dbus_connection_send_with_reply(
                    libdbusConnection,
                    libdbusMessageCall,
                    &libdbusPendingCall,
                    500);

    dbus_pending_call_set_notify(
                    libdbusPendingCall,
                    notifyThunk,
                    libdbusConnection,
                    NULL);

    ASSERT_EQ(true, future.get());
    dispatchThread.join();
}


std::promise<bool> promise2;
std::future<bool> future2 = promise2.get_future();
std::promise<bool> promise3;
std::future<bool> future3 = promise3.get_future();

void noPartnerCallback(DBusPendingCall*, void* data) {
	::DBusConnection* libdbusConnection = reinterpret_cast<DBusConnection*>(data);
	dbus_connection_close(libdbusConnection);
	dbus_connection_unref(libdbusConnection);
	promise2.set_value(true);
}

void noPartnerCleanup(void* data) {
	std::cout << "Cleanup" << std::endl;
	promise3.set_value(true);
}

TEST_F(DBusConnectionTest, TimeoutForNonexistingServices) {
	const ::DBusBusType libdbusType = ::DBusBusType::DBUS_BUS_SESSION;
	::DBusError libdbusError;
	dbus_error_init(&libdbusError);
	::DBusConnection* libdbusConnection = dbus_bus_get_private(libdbusType, &libdbusError);

	assert(libdbusConnection);
	dbus_connection_set_exit_on_disconnect(libdbusConnection, false);

	auto dispatchThread = std::thread(&dispatch, libdbusConnection);

	::DBusMessage* libdbusMessageCall = dbus_message_new_method_call(
			"some.connection.somewhere",
			"/some/non/existing/object",
			"some.interface.somewhere.but.same.place",
            "NoReasonableMethod");

	dbus_message_set_signature(libdbusMessageCall, "");

    bool hasHappened = false;

    DBusPendingCall* libdbusPendingCall;
    dbus_bool_t libdbusSuccess;

    dbus_connection_send_with_reply(
                    libdbusConnection,
                    libdbusMessageCall,
                    &libdbusPendingCall,
                    5000);

    dbus_pending_call_set_notify(
                    libdbusPendingCall,
                    noPartnerCallback,
                    libdbusConnection,
                    noPartnerCleanup);

    ASSERT_EQ(true, future2.get());
    dispatchThread.join();
}

//TEST_F(DBusConnectionTest, ConnectionsMayCommitAsynchronousSuicide) {
//	CommonAPI::DBus::DBusConnection* dbusConnection_ = new CommonAPI::DBus::DBusConnection(CommonAPI::DBus::DBusConnection::BusType::SESSION);
//	dbusConnection_->connect();
//
//    auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
//			"org.freedesktop.DBus",
//			"/org/freedesktop/DBus",
//			"org.freedesktop.DBus",
//            "ListNames",
//            "");
//
//    bool hasHappened = false;
//
//	auto future = dbusConnection_->sendDBusMessageWithReplyAsync(dbusMessageCall, CommonAPI::DBus::DBusProxyAsyncCallbackHandler<std::vector<std::string>>::create(
//    		[&] (const CommonAPI::CallStatus&, std::vector<std::string>) {
//				hasHappened = true;
//    			delete dbusConnection_;
//    		}
//    ));
//
//	ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, future.get());
//}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

