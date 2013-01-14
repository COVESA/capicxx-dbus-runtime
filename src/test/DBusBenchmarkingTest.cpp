/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusOutputStream.h>

#include <unordered_map>
#include <bits/functional_hash.h>

#include <gtest/gtest.h>

#include <dbus/dbus.h>

#include <chrono>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <bits/functional_hash.h>


class BenchmarkingTest: public ::testing::Test {
  protected:
    DBusMessage* libdbusMessage;
    DBusMessageIter libdbusMessageWriteIter;
    size_t numOfElements;

    void SetUp() {
        numOfElements = 2;
        libdbusMessage = dbus_message_new_method_call("no.bus.here", "/no/object/here", NULL, "noMethodHere");
        ASSERT_TRUE(libdbusMessage != NULL);
        dbus_message_iter_init_append(libdbusMessage, &libdbusMessageWriteIter);
    }

    void TearDown() {
        dbus_message_unref(libdbusMessage);
    }
};

template <typename _ArrayElementLibdbusType, typename _ArrayElementCommonApiType>
void prepareLibdbusArray(DBusMessage* libdbusMessage,
                         const int arrayElementLibdbusType,
                         const char* arrayElementLibdbusTypeAsString,
                         const _ArrayElementLibdbusType& arrayInitElementValue,
                         const uint32_t arrayInitTime,
                         size_t& libdbusInitElementCount) {

    DBusMessageIter libdbusMessageIter;
    DBusMessageIter libdbusMessageContainerIter;

    dbus_message_iter_init_append(libdbusMessage, &libdbusMessageIter);

    dbus_bool_t libdbusSuccess = dbus_message_iter_open_container(&libdbusMessageIter,
                                                                  DBUS_TYPE_ARRAY,
                                                                  arrayElementLibdbusTypeAsString,
                                                                  &libdbusMessageContainerIter);
    ASSERT_TRUE(libdbusSuccess);


    std::chrono::milliseconds libdbusInitTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> clockStart = std::chrono::high_resolution_clock::now();

    do {
        for (int i = 0; i < 1000; i++)
            dbus_message_iter_append_basic(&libdbusMessageContainerIter,
                                           arrayElementLibdbusType,
                                           &arrayInitElementValue);

        libdbusInitElementCount += 1000;

        libdbusInitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - clockStart);
    } while (libdbusInitTime.count() < arrayInitTime);

    libdbusSuccess = dbus_message_iter_close_container(&libdbusMessageIter, &libdbusMessageContainerIter);
    ASSERT_TRUE(libdbusSuccess);

    std::cout << "LibdbusInitTime = " << libdbusInitTime.count() << std::endl;
    std::cout << "LibdbusInitElementCount = " << libdbusInitElementCount << std::endl;
}


template <typename _ArrayElementLibdbusType, typename _ArrayElementCommonApiType>
void measureLibdbusArrayReadTime(DBusMessage* libdbusMessage,
                                 size_t& libdbusInitElementCount,
                                 std::chrono::milliseconds& libdbusArrayReadTime) {

    DBusMessageIter libdbusMessageIter;
    DBusMessageIter libdbusMessageContainerIter;

    dbus_bool_t libdbusSuccess;

    libdbusSuccess = dbus_message_iter_init(libdbusMessage, &libdbusMessageIter);
    ASSERT_TRUE(libdbusSuccess);

    dbus_message_iter_recurse(&libdbusMessageIter, &libdbusMessageContainerIter);

    size_t libdbusReadElementCount = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> clockStart = std::chrono::high_resolution_clock::now();

    do {
        _ArrayElementLibdbusType libdbusValue;

        dbus_message_iter_get_basic(&libdbusMessageContainerIter, &libdbusValue);

        ++libdbusReadElementCount;
    } while (dbus_message_iter_next(&libdbusMessageContainerIter));

    libdbusArrayReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - clockStart);

    ASSERT_EQ(libdbusInitElementCount, libdbusReadElementCount);
}


template <typename _ArrayElementLibdbusType, typename _ArrayElementCommonApiType>
void measureCommonApiArrayReadTime(DBusMessage* libdbusMessage,
                                   std::chrono::milliseconds& commonArrayApiReadTime,
                                   size_t& libdbusInitElementCount) {

    CommonAPI::DBus::DBusMessage dbusMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream DBusInputStream(dbusMessage);
    std::vector<_ArrayElementCommonApiType> commonApiVector;

    std::chrono::time_point<std::chrono::high_resolution_clock> clockStart = std::chrono::high_resolution_clock::now();

    DBusInputStream >> commonApiVector;

    commonArrayApiReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - clockStart);

    ASSERT_EQ(libdbusInitElementCount, commonApiVector.size());
}


template <typename _ArrayElementLibdbusType, typename _ArrayElementCommonApiType>
void measureArrayReadTime(
		const int arrayElementLibdbusType,
		const char* arrayElementLibdbusTypeAsString,
		const _ArrayElementLibdbusType arrayInitElementValue,
		const uint32_t arrayInitTime,
		std::chrono::milliseconds& libdbusArrayReadTime,
		std::chrono::milliseconds& commonArrayApiReadTime) {

	DBusMessage* libdbusMessage;
    dbus_bool_t libdbusSuccess;
    size_t libdbusInitElementCount = 0;

    libdbusMessage = dbus_message_new_method_call("no.bus.here", "/no/object/here", NULL, "noMethodHere");
    ASSERT_TRUE(libdbusMessage != NULL);

    prepareLibdbusArray<_ArrayElementLibdbusType, _ArrayElementCommonApiType>(
                                libdbusMessage,
                                arrayElementLibdbusType,
                                arrayElementLibdbusTypeAsString,
                                arrayInitElementValue,
                                arrayInitTime,
                                libdbusInitElementCount);

    measureLibdbusArrayReadTime<_ArrayElementLibdbusType, _ArrayElementCommonApiType>(
                                libdbusMessage,
                                libdbusInitElementCount,
                                libdbusArrayReadTime);

    measureCommonApiArrayReadTime<_ArrayElementLibdbusType, _ArrayElementCommonApiType>(
                                libdbusMessage,
                                commonArrayApiReadTime,
                                libdbusInitElementCount);
}


TEST_F(BenchmarkingTest, InputStreamReadsIntegerArrayFasterThanLibdbus) {
	std::chrono::milliseconds libdbusArrayReadTime;
	std::chrono::milliseconds commonApiArrayReadTime;

	measureArrayReadTime<int32_t, int32_t>(DBUS_TYPE_INT32,
										   DBUS_TYPE_INT32_AS_STRING,
										   1234567890,
										   1000,
										   libdbusArrayReadTime,
										   commonApiArrayReadTime);

	RecordProperty("LibdbusArrayReadTime", libdbusArrayReadTime.count());
	RecordProperty("CommonApiArrayReadTime", commonApiArrayReadTime.count());

	std::cout << "LibdbusArrayReadTime = " << libdbusArrayReadTime.count() << std::endl;
	std::cout << "CommonApiArrayReadTime = " << commonApiArrayReadTime.count() << std::endl;

    ASSERT_LT(commonApiArrayReadTime.count(), libdbusArrayReadTime.count() * 0.30)
    	<< "CommonAPI::DBus::DBusInputStream must be at least 70% faster than libdbus!";
}

TEST_F(BenchmarkingTest, InputStreamReadsStringArrayFasterThanLibdbus) {
	std::chrono::milliseconds libdbusArrayReadTime;
	std::chrono::milliseconds commonApiArrayReadTime;

	measureArrayReadTime<char*, std::string>(DBUS_TYPE_STRING,
											 DBUS_TYPE_STRING_AS_STRING,
											 const_cast<char*>("01234567890123456789"),
											 1000,
											 libdbusArrayReadTime,
											 commonApiArrayReadTime);

	RecordProperty("LibdbusArrayReadTime", libdbusArrayReadTime.count());
	RecordProperty("CommonApiArrayReadTime", commonApiArrayReadTime.count());

	std::cout << "LibdbusArrayReadTime = " << libdbusArrayReadTime.count() << std::endl;
	std::cout << "CommonApiArrayReadTime = " << commonApiArrayReadTime.count() << std::endl;

    ASSERT_LT(commonApiArrayReadTime.count(), libdbusArrayReadTime.count() * 0.30)
    	<< "CommonAPI::DBus::DBusInputStream must be at least 70% faster than libdbus!";
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
