/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/SerializableVariant.h>
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


class InputStreamTest: public ::testing::Test {
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

TEST_F(InputStreamTest, CanBeConstructed) {
    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);
}

TEST_F(InputStreamTest, ReadsEmptyMessages) {
    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(0, scopedMessage.getBodyLength());
}

TEST_F(InputStreamTest, ReadsBytes) {
    uint8_t val = 0xff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BYTE, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        uint8_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsBools) {
    dbus_bool_t f = FALSE;
    dbus_bool_t t = TRUE;
    for (int i = 0; i < numOfElements; i += 2) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BOOLEAN, &t);
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BOOLEAN, &f);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 2) {
        bool readVal;
        inStream >> readVal;
        EXPECT_EQ(t, readVal);
        inStream >> readVal;
        EXPECT_EQ(f, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint16) {

    uint16_t val = 0xffff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT16, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*2, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        uint16_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt16) {

    int16_t val = 0x7fff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT16, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*2, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        int16_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint32) {

    uint32_t val = 0xffffffff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT32, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        uint32_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt32) {

    int32_t val = 0x7fffffff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT32, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        int32_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint64) {

    uint64_t val = 0xffffffffffffffff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT64, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        uint64_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt64) {

    int64_t val = 0x7fffffffffffffff;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT64, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        int64_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsDoubles) {

    double val = 13.37;
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_DOUBLE, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        double readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsStrings) {

    std::string val = "hai";
    for (int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_STRING, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4 + numOfElements*4, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        std::string readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}



namespace bmw {
namespace test {

struct TestSerializableStruct: CommonAPI::SerializableStruct {
    uint32_t a;
    int16_t b;
    bool c;
    std::string d;
    double e;

	virtual void readFromInputStream(CommonAPI::InputStream& inputStream) {
		inputStream >> a >> b >> c >> d >> e;
	}

	virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const {
		outputStream << a << b << c << d << e;
	}
};

} //namespace test
} //namespace bmw

TEST_F(InputStreamTest, ReadsStructs) {

    bmw::test::TestSerializableStruct testStruct;
    testStruct.a = 15;
    testStruct.b = -32;
    testStruct.c = FALSE;
    testStruct.d = "Hello all";
    testStruct.e = 3.414;

    DBusMessageIter subIter;
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_UINT32, &testStruct.a);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_INT16, &testStruct.b);
    dbus_bool_t dbusBool = static_cast<dbus_bool_t>(testStruct.c);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BOOLEAN, &dbusBool);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_STRING, &testStruct.d);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_DOUBLE, &testStruct.e);
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    //40(byte length of struct) = 4(uint32_t) + 2(int16_t) + 2(padding) + 4(bool) + 4(strlength)
    //                          + 9(string) + 1(terminating '\0' of string) + 6(padding) + 8 (double)
    EXPECT_EQ(40, scopedMessage.getBodyLength());

    bmw::test::TestSerializableStruct verifyStruct;
    inStream >> verifyStruct;
    EXPECT_EQ(testStruct.a, verifyStruct.a);
    EXPECT_EQ(testStruct.b, verifyStruct.b);
    EXPECT_EQ(testStruct.c, verifyStruct.c);
    EXPECT_EQ(testStruct.d, verifyStruct.d);
    EXPECT_EQ(testStruct.e, verifyStruct.e);
}

TEST_F(InputStreamTest, ReadsArrays) {

    std::vector<int32_t> testVector;
    int32_t val1 = 0xffffffff;
    int32_t val2 = 0x7fffffff;
    for (int i = 0; i < numOfElements; i += 2) {
        testVector.push_back(val1);
        testVector.push_back(val2);
    }

    DBusMessageIter subIter;
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_ARRAY, "i", &subIter);
    for (int i = 0; i < numOfElements; i++) {
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_INT32, &testVector[i]);
    }
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(scopedMessage.getBodyLength(), numOfElements*4 + 4);
    std::vector<int32_t> verifyVector;
    inStream >> verifyVector;
    int32_t res1;
    int32_t res2;
    for (int i = 0; i < numOfElements; i += 2) {
        res1 = verifyVector[i];
        EXPECT_EQ(val1, res1);
        res2 = verifyVector[i + 1];
        EXPECT_EQ(val2, res2);
    }
}

TEST_F(InputStreamTest, ReadsArraysInArrays) {
    size_t numOfElements = 2;
    DBusMessage* dbusMessage = dbus_message_new_method_call("no.bus.here", "/no/object/here", NULL, "noMethodHere");
    ASSERT_TRUE(dbusMessage != NULL);

    DBusMessageIter writeIter;
    dbus_message_iter_init_append(dbusMessage, &writeIter);

    std::vector<std::vector<int32_t>> testVector;
    int32_t val1 = 0xffffffff;
    int32_t val2 = 0x7fffffff;
    for (int i = 0; i < numOfElements; i++) {
        std::vector<int32_t> inner;
        for (int j = 0; j < numOfElements; j += 2) {
            inner.push_back(val1);
            inner.push_back(val2);
        }
        testVector.push_back(inner);
    }

    DBusMessageIter subIter;
    dbus_message_iter_open_container(&writeIter, DBUS_TYPE_ARRAY, "ai", &subIter);
    for (int i = 0; i < numOfElements; i++) {
        DBusMessageIter subsubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_ARRAY, "i", &subsubIter);
        for (int j = 0; j < numOfElements; j++) {
            dbus_message_iter_append_basic(&subsubIter, DBUS_TYPE_INT32, &(testVector[i][j]));
        }
        dbus_message_iter_close_container(&subIter, &subsubIter);
    }
    dbus_message_iter_close_container(&writeIter, &subIter);

    CommonAPI::DBus::DBusMessage scopedMessage(dbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    // 5*5*int32_t + 5*lengthField_inner(int32_t) + lengthField_outer(int32_t)
    EXPECT_EQ(numOfElements*numOfElements*4 + numOfElements*4 + 4, scopedMessage.getBodyLength());
    std::vector<std::vector<int32_t>> verifyVector;
    inStream >> verifyVector;

    int32_t res1;
    int32_t res2;
    for (int i = 0; i < numOfElements; i++) {
        std::vector<int32_t> innerVerify = verifyVector[i];
        for (int j = 0; j < numOfElements; j += 2) {
            res1 = innerVerify[j];
            EXPECT_EQ(val1, res1);
            res2 = innerVerify[j + 1];
            EXPECT_EQ(val2, res2);
        }
    }
}

TEST_F(InputStreamTest, ReadsBasicVariants) {
    int32_t fromInt = 5;
    int8_t varIndex = 3;

    for (int i = 0; i < numOfElements; i += 1) {
        DBusMessageIter subIter;
        dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &varIndex);
        DBusMessageIter subSubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "i", &subSubIter);
        dbus_message_iter_append_basic(&subSubIter, DBUS_TYPE_INT32, &fromInt);
        dbus_message_iter_close_container(&subIter, &subSubIter);
        dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    CommonAPI::Variant<int32_t, double, std::string> referenceVariant(fromInt);

    bool success;
    int32_t referenceResult = referenceVariant.get<int32_t>(success);
    EXPECT_TRUE(success);

    EXPECT_EQ(numOfElements*4 + numOfElements*4, scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        CommonAPI::Variant<int32_t, double, std::string> readVariant;
        inStream >> readVariant;

        bool readSuccess;
        int32_t actualResult = readVariant.get<int32_t>(readSuccess);
        EXPECT_TRUE(readSuccess);

        bool varEq = (referenceVariant == readVariant);
        EXPECT_TRUE(varEq);
        EXPECT_EQ(referenceResult, actualResult);
    }
}

TEST_F(InputStreamTest, ReadsStringVariants) {
    std::string fromString = "Hello World with CommonAPI Variants!";
    int8_t varIndex = 1;

    for (int i = 0; i < numOfElements; i += 1) {
        DBusMessageIter subIter;
        dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &varIndex);
        DBusMessageIter subSubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "s", &subSubIter);
        dbus_message_iter_append_basic(&subSubIter, DBUS_TYPE_STRING, &fromString);
        dbus_message_iter_close_container(&subIter, &subSubIter);
        dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    CommonAPI::Variant<int32_t, double, std::string> referenceVariant(fromString);

    bool success;
    std::string referenceResult = referenceVariant.get<std::string>(success);
    EXPECT_TRUE(success);

    //Variant: type-index(1) + padding(3) + stringLength(4) + string(37) = 45
    //         +struct-padding inbetween (alignment 8)
    EXPECT_EQ(numOfElements * (1+3+4+fromString.length()+1) + (numOfElements - 1) * (8-((fromString.length()+1)%8)) , scopedMessage.getBodyLength());
    for (int i = 0; i < numOfElements; i += 1) {
        CommonAPI::Variant<int32_t, double, std::string> readVariant;
        inStream >> readVariant;

        bool readSuccess;
        std::string actualResult = readVariant.get<std::string>(readSuccess);
        EXPECT_TRUE(readSuccess);

        bool variantsAreEqual = (referenceVariant == readVariant);
        EXPECT_TRUE(variantsAreEqual);
        EXPECT_EQ(referenceResult, actualResult);
    }
}


int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
