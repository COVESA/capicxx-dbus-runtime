/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <common-api-dbus/dbus-connection.h>
#include <common-api-dbus/dbus-output-message-stream.h>

#include <gtest/gtest.h>

#include <cstring>


#define	ASSERT_DBUSMESSAGE_EQ(_dbusMessage1, _dbusMessage2) \
			ASSERT_FALSE(_dbusMessage1.getSignatureString() == NULL); \
			ASSERT_FALSE(_dbusMessage2.getSignatureString() == NULL); \
			ASSERT_STREQ(_dbusMessage1.getSignatureString(), _dbusMessage2.getSignatureString()); \
			ASSERT_EQ(_dbusMessage1.getBodyLength(), _dbusMessage2.getBodyLength()); \
			ASSERT_FALSE(_dbusMessage1.getBodyData() == NULL); \
			ASSERT_FALSE(_dbusMessage2.getBodyData() == NULL); \
			ASSERT_EQ(memcmp(_dbusMessage1.getBodyData(), _dbusMessage2.getBodyData(), _dbusMessage1.getBodyLength()), 0)


namespace {

class DBusConnectionTest: public ::testing::Test {
 public:
	void onConnectionStatusEvent(const common::api::AvailabilityStatus& newConnectionStatus) {
		connectionStatusEventCount_++;
		connectionStatus_ = newConnectionStatus;
	}

	bool onInterfaceHandlerDBusMessageReply(const common::api::dbus::DBusMessage& dbusMessage,
											const std::shared_ptr<common::api::dbus::DBusConnection>& dbusConnection) {
		interfaceHandlerDBusMessageCount_++;
		interfaceHandlerDBusMessage_ = dbusMessage;
		interfaceHandlerDBusMessageReply_ = dbusMessage.createMethodReturn("si");

		common::api::dbus::DBusOutputMessageStream dbusOutputMessageStream(interfaceHandlerDBusMessageReply_);
		dbusOutputMessageStream << "This is a default message reply!" << interfaceHandlerDBusMessageCount_;
		dbusOutputMessageStream.flush();

		dbusConnection->sendDBusMessage(interfaceHandlerDBusMessageReply_);

		return true;
	}

	void onDBusMessageHandler(const common::api::dbus::DBusMessage& dbusMessage) {
		dbusMessageHandlerCount_++;
		dbusMessageHandlerDBusMessage_ = dbusMessage;
	}

 protected:
	virtual void SetUp() {
		dbusConnection_ = common::api::dbus::DBusConnection::getSessionBus();
		connectionStatusEventCount_ = 0;
		interfaceHandlerDBusMessageCount_ = 0;
		dbusMessageHandlerCount_ = 0;
	}

	virtual void TearDown() {
		if (dbusConnection_ && dbusConnection_->isConnected())
			dbusConnection_->disconnect();

		// reset DBusMessage
		interfaceHandlerDBusMessage_ = common::api::dbus::DBusMessage();
		interfaceHandlerDBusMessageReply_ = common::api::dbus::DBusMessage();

		dbusMessageHandlerDBusMessage_ = common::api::dbus::DBusMessage();
	}


	std::shared_ptr<common::api::dbus::DBusConnection> dbusConnection_;

	uint32_t connectionStatusEventCount_;
	common::api::AvailabilityStatus connectionStatus_;

	uint32_t interfaceHandlerDBusMessageCount_;
	common::api::dbus::DBusMessage interfaceHandlerDBusMessage_;
	common::api::dbus::DBusMessage interfaceHandlerDBusMessageReply_;

	uint32_t dbusMessageHandlerCount_;
	common::api::dbus::DBusMessage dbusMessageHandlerDBusMessage_;
};


TEST_F(DBusConnectionTest, IsInitiallyDisconnected) {
	ASSERT_FALSE(dbusConnection_->isConnected());
}

TEST_F(DBusConnectionTest, ConnectAndDisconnectWork) {
	ASSERT_TRUE(dbusConnection_->connect());
	ASSERT_TRUE(dbusConnection_->isConnected());

	dbusConnection_->disconnect();
	ASSERT_FALSE(dbusConnection_->isConnected());
}

TEST_F(DBusConnectionTest, ConnectionStatusEventWorks) {
	ASSERT_EQ(connectionStatusEventCount_, 0);

	auto connectionStatusSubscription = dbusConnection_->getConnectionStatusEvent().subscribe(std::bind(
			&DBusConnectionTest::onConnectionStatusEvent,
			this,
			std::placeholders::_1));

	ASSERT_FALSE(dbusConnection_->isConnected());
	ASSERT_EQ(connectionStatusEventCount_, 0);

	uint32_t expectedEventCount = 0;
	while (expectedEventCount < 10) {
		ASSERT_TRUE(dbusConnection_->connect());
		ASSERT_TRUE(dbusConnection_->isConnected());
		ASSERT_EQ(connectionStatusEventCount_, ++expectedEventCount);
		ASSERT_EQ(connectionStatus_, common::api::AvailabilityStatus::AVAILABLE);

		dbusConnection_->disconnect();
		ASSERT_FALSE(dbusConnection_->isConnected());
		ASSERT_EQ(connectionStatusEventCount_, ++expectedEventCount);
		ASSERT_EQ(connectionStatus_, common::api::AvailabilityStatus::NOT_AVAILABLE);
	}

	dbusConnection_->getConnectionStatusEvent().unsubscribe(connectionStatusSubscription);
	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);

	ASSERT_TRUE(dbusConnection_->connect());
	ASSERT_TRUE(dbusConnection_->isConnected());
	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);

	dbusConnection_->disconnect();
	ASSERT_FALSE(dbusConnection_->isConnected());
	ASSERT_EQ(connectionStatusEventCount_, expectedEventCount);
}

TEST_F(DBusConnectionTest, SendingAsyncDBusMessagesWorks) {
	const char* busName = "common.api.dbus.test.TestInterfaceHandler";
	const char* objectPath = "/common/api/dbus/test/TestObject";
	const char* interfaceName = "common.api.dbus.test.TestInterface";
	const char* methodName = "TestMethod";

	auto interfaceHandlerDBusConnection = common::api::dbus::DBusConnection::getSessionBus();

	ASSERT_TRUE(interfaceHandlerDBusConnection->connect());
	ASSERT_TRUE(interfaceHandlerDBusConnection->requestServiceNameAndBlock(busName));

	auto interfaceHandlerToken = interfaceHandlerDBusConnection->registerInterfaceHandler(
			objectPath,
			interfaceName,
			std::bind(&DBusConnectionTest::onInterfaceHandlerDBusMessageReply,
					  this,
					  std::placeholders::_1,
					  interfaceHandlerDBusConnection));


	ASSERT_TRUE(dbusConnection_->connect());

	for (uint32_t expectedDBusMessageCount = 1; expectedDBusMessageCount <= 10; expectedDBusMessageCount++) {
		auto dbusMessageCall = common::api::dbus::DBusMessage::createMethodCall(
				busName,
				objectPath,
				interfaceName,
				methodName,
				"si");
		ASSERT_TRUE(dbusMessageCall);

		common::api::dbus::DBusOutputMessageStream dbusOutputMessageStream(dbusMessageCall);
		dbusOutputMessageStream << "This is a test async call"
								<< expectedDBusMessageCount;
		dbusOutputMessageStream.flush();

		dbusConnection_->sendDBusMessageWithReplyAsync(
				dbusMessageCall,
				std::bind(&DBusConnectionTest::onDBusMessageHandler, this, std::placeholders::_1));

		for (int i = 0; i < 10 && interfaceHandlerDBusMessageCount_ < expectedDBusMessageCount; i++)
			interfaceHandlerDBusConnection->readWriteDispatch(100);

		ASSERT_EQ(interfaceHandlerDBusMessageCount_, expectedDBusMessageCount);
		ASSERT_DBUSMESSAGE_EQ(dbusMessageCall, interfaceHandlerDBusMessage_);

		for (int i = 0; i < 10 && dbusMessageHandlerCount_ < expectedDBusMessageCount; i++)
			dbusConnection_->readWriteDispatch(100);

		ASSERT_EQ(dbusMessageHandlerCount_, expectedDBusMessageCount);
		ASSERT_DBUSMESSAGE_EQ(dbusMessageHandlerDBusMessage_, interfaceHandlerDBusMessageReply_);
	}

	dbusConnection_->disconnect();


	interfaceHandlerDBusConnection->unregisterInterfaceHandler(interfaceHandlerToken);

	ASSERT_TRUE(interfaceHandlerDBusConnection->releaseServiceName(busName));
	interfaceHandlerDBusConnection->disconnect();
}

} // namespace
