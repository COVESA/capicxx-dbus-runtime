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


bool replyArrived;

class LibdbusTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


::DBusHandlerResult onLibdbusObjectPathMessageThunk(::DBusConnection* libdbusConnection,
                                                    ::DBusMessage* libdbusMessage,
                                                    void* userData) {
   return ::DBusHandlerResult::DBUS_HANDLER_RESULT_HANDLED;
}

DBusObjectPathVTable libdbusObjectPathVTable = {
               NULL,
               &onLibdbusObjectPathMessageThunk
};

::DBusConnection* createConnection() {
   const ::DBusBusType libdbusType = ::DBusBusType::DBUS_BUS_SESSION;
   ::DBusConnection* libdbusConnection = dbus_bus_get_private(libdbusType, NULL);
   dbus_connection_ref(libdbusConnection);
   dbus_connection_set_exit_on_disconnect(libdbusConnection, false);

   return libdbusConnection;
}

static void onLibdbusPendingCallNotifyThunk(::DBusPendingCall* libdbusPendingCall, void *userData) {
    replyArrived = true;
    ::DBusMessage* libdbusMessage = dbus_pending_call_steal_reply(libdbusPendingCall);
    ASSERT_TRUE(libdbusMessage);
    dbus_pending_call_unref(libdbusPendingCall);
}

TEST_F(LibdbusTest, DISABLED_NonreplyingLibdbusConnectionsAreHandled) {
    const char problemServiceName[] = "problem.service";
    replyArrived = false;
    bool running = true;

    dbus_threads_init_default();

   ::DBusConnection* serviceConnection = createConnection();
   ::DBusConnection* clientConnection = createConnection();


   dbus_bus_request_name(serviceConnection,
                   problemServiceName,
                   DBUS_NAME_FLAG_DO_NOT_QUEUE,
                   NULL);

   dbus_connection_try_register_object_path(serviceConnection,
                   "/",
                   &libdbusObjectPathVTable,
                   NULL,
                   NULL);

   std::thread([&, this] {
       while(running) {
           dbus_connection_read_write_dispatch(serviceConnection, 10);
       }
   }).detach();

   usleep(100000);

   ::DBusMessage* message = dbus_message_new_method_call(problemServiceName, "/", NULL, "someMethod");

   ::DBusPendingCall* libdbusPendingCall;

   dbus_connection_send_with_reply(
                   clientConnection,
                   message,
                   &libdbusPendingCall,
                   3000);

   dbus_pending_call_set_notify(
                   libdbusPendingCall,
                   onLibdbusPendingCallNotifyThunk,
                   NULL,
                   NULL);

   //100*50 = 5000 (ms) ==> 3 seconds timeout pending call *should* have arrived by now.
   for (unsigned int i = 0; i < 100 && (!replyArrived); i++) {
       dbus_connection_read_write_dispatch(clientConnection, 50);
   }

   EXPECT_TRUE(replyArrived);

   running = false;

   usleep(100000);

   dbus_connection_close(serviceConnection);
   dbus_connection_unref(serviceConnection);
   dbus_connection_close(clientConnection);
   dbus_connection_unref(clientConnection);
}


class DBusConnectionTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        dbusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
    }

    virtual void TearDown() {
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
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
    uint32_t connectionStatusEventCount = 0;
    CommonAPI::AvailabilityStatus connectionStatus = CommonAPI::AvailabilityStatus::UNKNOWN;

    auto connectionStatusSubscription = dbusConnection_->getConnectionStatusEvent().subscribe(std::bind(
                    [&connectionStatusEventCount, &connectionStatus](CommonAPI::AvailabilityStatus availabilityStatus) {
                        ++connectionStatusEventCount;
                        connectionStatus = availabilityStatus;
                    },
                    std::placeholders::_1));

    ASSERT_FALSE(dbusConnection_->isConnected());
    ASSERT_EQ(connectionStatusEventCount, 0);

    uint32_t expectedEventCount = 0;
    while (expectedEventCount < 10) {
        ASSERT_TRUE(dbusConnection_->connect());
        ASSERT_TRUE(dbusConnection_->isConnected());
        usleep(20000);
        ASSERT_EQ(connectionStatusEventCount, ++expectedEventCount);
        ASSERT_EQ(connectionStatus, CommonAPI::AvailabilityStatus::AVAILABLE);

        dbusConnection_->disconnect();
        ASSERT_FALSE(dbusConnection_->isConnected());
        usleep(20000);
        ASSERT_EQ(connectionStatusEventCount, ++expectedEventCount);
        ASSERT_EQ(connectionStatus, CommonAPI::AvailabilityStatus::NOT_AVAILABLE);
    }

    dbusConnection_->getConnectionStatusEvent().unsubscribe(connectionStatusSubscription);
    ASSERT_EQ(connectionStatusEventCount, expectedEventCount);

    ASSERT_TRUE(dbusConnection_->connect());
    ASSERT_TRUE(dbusConnection_->isConnected());
    ASSERT_EQ(connectionStatusEventCount, expectedEventCount);

    dbusConnection_->disconnect();
    ASSERT_FALSE(dbusConnection_->isConnected());
    ASSERT_EQ(connectionStatusEventCount, expectedEventCount);
}

TEST_F(DBusConnectionTest, SendingAsyncDBusMessagesWorks) {
    const char busName[] = "commonapi.dbus.test.TestInterfaceHandler";
    const char objectPath[] = "/common/api/dbus/test/TestObject";
    const char interfaceName[] = "commonapi.dbus.test.TestInterface";
    const char methodName[] = "TestMethod";

    auto interfaceHandlerDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();

    ASSERT_TRUE(interfaceHandlerDBusConnection->connect());
    ASSERT_TRUE(interfaceHandlerDBusConnection->requestServiceNameAndBlock(busName));

    uint32_t serviceHandlerDBusMessageCount = 0;
    uint32_t clientReplyHandlerDBusMessageCount = 0;

    interfaceHandlerDBusConnection->setObjectPathMessageHandler(
                    [&serviceHandlerDBusMessageCount, &interfaceHandlerDBusConnection] (CommonAPI::DBus::DBusMessage dbusMessage) -> bool {
                        ++serviceHandlerDBusMessageCount;
                        CommonAPI::DBus::DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("");
                        interfaceHandlerDBusConnection->sendDBusMessage(dbusMessageReply);
                        return true;
                    }
                    );

    interfaceHandlerDBusConnection->registerObjectPath(objectPath);

    ASSERT_TRUE(dbusConnection_->connect());

    CommonAPI::DBus::DBusMessage dbusReplyMessage;

    for (uint32_t expectedDBusMessageCount = 1; expectedDBusMessageCount <= 10; expectedDBusMessageCount++) {
        CommonAPI::DBus::DBusMessage dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                        busName,
                        objectPath,
                        interfaceName,
                        methodName,
                        "");

        CommonAPI::DBus::DBusOutputStream dbusOutputStream(dbusMessageCall);

        interfaceHandlerDBusConnection->sendDBusMessageWithReplyAsync(
                        dbusMessageCall,
                        CommonAPI::DBus::DBusProxyAsyncCallbackHandler<>::create(
                                        [&clientReplyHandlerDBusMessageCount](CommonAPI::CallStatus status) {
                                            ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);
                                            ++clientReplyHandlerDBusMessageCount;
                                        })
                                        );

        for (int i = 0; i < 10 && serviceHandlerDBusMessageCount < expectedDBusMessageCount; i++) {
            interfaceHandlerDBusConnection->readWriteDispatch(100);
        }

        ASSERT_EQ(serviceHandlerDBusMessageCount, expectedDBusMessageCount);

        for (int i = 0; i < 10 && clientReplyHandlerDBusMessageCount < expectedDBusMessageCount; i++) {
            dbusConnection_->readWriteDispatch(100);
        }

        ASSERT_EQ(clientReplyHandlerDBusMessageCount, expectedDBusMessageCount);
    }

    dbusConnection_->disconnect();

    interfaceHandlerDBusConnection->unregisterObjectPath(objectPath);

    ASSERT_TRUE(interfaceHandlerDBusConnection->releaseServiceName(busName));
    interfaceHandlerDBusConnection->disconnect();
}

void dispatch(::DBusConnection* libdbusConnection) {
    dbus_bool_t success = TRUE;
    while (success) {
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

// libdbus bug
TEST_F(DBusConnectionTest, DISABLED_TimeoutForNonexistingServices) {
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

