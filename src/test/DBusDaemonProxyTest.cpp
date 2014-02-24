/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusDaemonProxy.h>
#include <CommonAPI/DBus/DBusUtils.h>

#include <gtest/gtest.h>

#include <future>
#include <tuple>

namespace {

class DBusDaemonProxyTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        dbusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(dbusConnection_->connect());
        dbusDaemonProxy_ = std::make_shared<CommonAPI::DBus::DBusDaemonProxy>(dbusConnection_);
    }

    virtual void TearDown() {
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusDaemonProxy> dbusDaemonProxy_;
};

TEST_F(DBusDaemonProxyTest, ListNames) {
    std::vector<std::string> busNames;
    CommonAPI::CallStatus callStatus;

    dbusDaemonProxy_->listNames(callStatus, busNames);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    ASSERT_GT(busNames.size(), 0);
    for (const std::string& busName : busNames) {
        ASSERT_FALSE(busName.empty());
        ASSERT_GT(busName.length(), 1);
    }
}

TEST_F(DBusDaemonProxyTest, ListNamesAsync) {
    std::promise<std::tuple<CommonAPI::CallStatus, std::vector<std::string>>>promise;
    auto future = promise.get_future();

    auto callStatusFuture = dbusDaemonProxy_->listNamesAsync(
                    [&](const CommonAPI::CallStatus& callStatus, std::vector<std::string> busNames) {
                        promise.set_value(std::tuple<CommonAPI::CallStatus, std::vector<std::string>>(callStatus, std::move(busNames)));
                    });

    auto status = future.wait_for(std::chrono::milliseconds(500));
    bool waitResult = CommonAPI::DBus::checkReady(status);
    ASSERT_EQ(waitResult, true);

    ASSERT_EQ(callStatusFuture.get(), CommonAPI::CallStatus::SUCCESS);

    auto futureResult = future.get();
    const CommonAPI::CallStatus& callStatus = std::get<0>(futureResult);
    const std::vector<std::string>& busNames = std::get<1>(futureResult);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    ASSERT_GT(busNames.size(), 0);
    for (const std::string& busName : busNames) {
        ASSERT_FALSE(busName.empty());
        ASSERT_GT(busName.length(), 1);
    }
}

TEST_F(DBusDaemonProxyTest, NameHasOwner) {
    bool nameHasOwner;
    CommonAPI::CallStatus callStatus;

    dbusDaemonProxy_->nameHasOwner("org.freedesktop.DBus", callStatus, nameHasOwner);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(nameHasOwner);

    dbusDaemonProxy_->nameHasOwner("org.freedesktop.DBus.InvalidName.XXYYZZ", callStatus, nameHasOwner);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_FALSE(nameHasOwner);
}

TEST_F(DBusDaemonProxyTest, NameHasOwnerAsync) {
    std::promise<std::tuple<CommonAPI::CallStatus, bool>> promise;
    auto future = promise.get_future();

    auto callStatusFuture = dbusDaemonProxy_->nameHasOwnerAsync(
                    "org.freedesktop.DBus",
                    [&](const CommonAPI::CallStatus& callStatus, bool nameHasOwner) {
                        promise.set_value(std::tuple<CommonAPI::CallStatus, bool>(callStatus, std::move(nameHasOwner)));
                    });

    auto status = future.wait_for(std::chrono::milliseconds(100));
    const bool waitResult = CommonAPI::DBus::checkReady(status);
    ASSERT_EQ(waitResult, true);

    ASSERT_EQ(callStatusFuture.get(), CommonAPI::CallStatus::SUCCESS);

    auto futureResult = future.get();
    const CommonAPI::CallStatus& callStatus = std::get<0>(futureResult);
    const bool& nameHasOwner = std::get<1>(futureResult);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(nameHasOwner);
}

TEST_F(DBusDaemonProxyTest, NameOwnerChangedEvent) {
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto subscription = dbusDaemonProxy_->getNameOwnerChangedEvent().subscribe(
                    [&](const std::string& name, const std::string& oldOwner, const std::string& newOwner) {
                        static bool promiseIsSet = false;
                        if(!promiseIsSet) {
                            promiseIsSet = true;
                            promise.set_value(!name.empty() && (!oldOwner.empty() || !newOwner.empty()));
                        }
                    });

    // Trigger NameOwnerChanged using a new DBusConnection
    ASSERT_TRUE(CommonAPI::DBus::DBusConnection::getSessionBus()->connect());

    ASSERT_TRUE(future.get());

    dbusDaemonProxy_->getNameOwnerChangedEvent().unsubscribe(subscription);
}

} // namespace

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
