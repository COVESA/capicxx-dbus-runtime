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

#include "DBusTestUtils.h"

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
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BYTE, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        uint8_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsBools) {
    dbus_bool_t f = FALSE;
    dbus_bool_t t = TRUE;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BOOLEAN, &t);
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_BOOLEAN, &f);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        bool readVal;
        inStream >> readVal;
        EXPECT_EQ(t, readVal);
        inStream >> readVal;
        EXPECT_EQ(f, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint16) {

    uint16_t val = 0xffff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT16, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*2, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        uint16_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt16) {

    int16_t val = 0x7fff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT16, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*2, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        int16_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint32) {

    uint32_t val = 0xffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT32, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        uint32_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt32) {

    int32_t val = 0x7fffffff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT32, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        int32_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsUint64) {

    uint64_t val = 0xffffffffffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_UINT64, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        uint64_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsInt64) {

    int64_t val = 0x7fffffffffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_INT64, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        int64_t readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsDoubles) {

    double val = 13.37;
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_DOUBLE, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*8, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        double readVal;
        inStream >> readVal;
        EXPECT_EQ(val, readVal);
    }
}

TEST_F(InputStreamTest, ReadsStrings) {

    std::string val = "hai";
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        dbus_message_iter_append_basic(&libdbusMessageWriteIter, DBUS_TYPE_STRING, &val);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    EXPECT_EQ(numOfElements*4 + numOfElements*4, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
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

    static void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
        typeOutputStream.writeUInt32Type();
        typeOutputStream.writeInt16Type();
        typeOutputStream.writeBoolType();
        typeOutputStream.writeStringType();
        typeOutputStream.writeDoubleType();
    }
};

bool operator==(const TestSerializableStruct& lhs, const TestSerializableStruct& rhs) {
    if (&lhs == &rhs)
        return true;

    return (lhs.a == rhs.a) && (lhs.b == rhs.b) && (lhs.c == rhs.c) && (lhs.d == rhs.d) && (lhs.e == rhs.e);
}

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
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        testVector.push_back(val1);
        testVector.push_back(val2);
    }

    DBusMessageIter subIter;
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_ARRAY, "i", &subIter);
    for (unsigned int i = 0; i < numOfElements; i++) {
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
    for (unsigned int i = 0; i < numOfElements; i += 2) {
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
    for (unsigned int i = 0; i < numOfElements; i++) {
        std::vector<int32_t> inner;
        for (unsigned int j = 0; j < numOfElements; j += 2) {
            inner.push_back(val1);
            inner.push_back(val2);
        }
        testVector.push_back(inner);
    }

    DBusMessageIter subIter;
    dbus_message_iter_open_container(&writeIter, DBUS_TYPE_ARRAY, "ai", &subIter);
    for (unsigned int i = 0; i < numOfElements; i++) {
        DBusMessageIter subsubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_ARRAY, "i", &subsubIter);
        for (unsigned int j = 0; j < numOfElements; j++) {
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
    for (unsigned int i = 0; i < numOfElements; i++) {
        std::vector<int32_t> innerVerify = verifyVector[i];
        for (unsigned int j = 0; j < numOfElements; j += 2) {
            res1 = innerVerify[j];
            EXPECT_EQ(val1, res1);
            res2 = innerVerify[j + 1];
            EXPECT_EQ(val2, res2);
        }
    }
}

TEST_F(InputStreamTest, ReadsInt32Variants) {
    typedef CommonAPI::Variant<int32_t, double, std::string> TestedVariantType;

    int32_t fromInt = 5;
    int8_t variantTypeIndex = 3;

    for (unsigned int i = 0; i < numOfElements; i += 1) {
        DBusMessageIter subIter;
        dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &variantTypeIndex);
        DBusMessageIter subSubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "i", &subSubIter);
        dbus_message_iter_append_basic(&subSubIter, DBUS_TYPE_INT32, &fromInt);
        dbus_message_iter_close_container(&subIter, &subSubIter);
        dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    TestedVariantType referenceVariant(fromInt);

    EXPECT_EQ(numOfElements*4 + numOfElements*4, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        TestedVariantType readVariant;
        inStream >> readVariant;

        int32_t actualResult;
        EXPECT_NO_THROW(actualResult = readVariant.get<int32_t>());

        bool varEq = (referenceVariant == readVariant);
        EXPECT_TRUE(varEq);
        EXPECT_EQ(fromInt, actualResult);
    }
}

TEST_F(InputStreamTest, ReadsStringVariants) {
    typedef CommonAPI::Variant<int32_t, double, std::string> TestedVariantType;

    std::string fromString = "Hello World with CommonAPI Variants!";
    int8_t variantTypeIndex = 1;

    for (unsigned int i = 0; i < numOfElements; i += 1) {
        DBusMessageIter subIter;
        dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &variantTypeIndex);
        DBusMessageIter subSubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "s", &subSubIter);
        dbus_message_iter_append_basic(&subSubIter, DBUS_TYPE_STRING, &fromString);
        dbus_message_iter_close_container(&subIter, &subSubIter);
        dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    TestedVariantType referenceVariant(fromString);

    //Variant: type-index(1) + signature(2) + padding(1) + stringLength(4) + string(37) = 45
    //         +struct-padding inbetween (alignment 8)
    EXPECT_EQ(numOfElements * (1+3+4+fromString.length()+1) + (numOfElements - 1) * (8-((fromString.length()+1)%8)) , scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        TestedVariantType readVariant;
        inStream >> readVariant;

        std::string actualResult = readVariant.get<std::string>();

        bool variantsAreEqual = (referenceVariant == readVariant);
        EXPECT_TRUE(variantsAreEqual);
        EXPECT_EQ(fromString, actualResult);
    }
}


TEST_F(InputStreamTest, ReadsVariantsWithAnArrayOfStrings) {
    typedef CommonAPI::Variant<int32_t, double, std::vector<std::string>> TestedVariantType;

    std::string testString1 = "Hello World with CommonAPI Variants!";
    std::string testString2 = "What a beautiful world if there are working Arrays within Variants!!";
    int8_t variantTypeIndex = 1;

    std::vector<std::string> testInnerVector;

    for (unsigned int i = 0; i < numOfElements; i += 2) {
        testInnerVector.push_back(testString1);
        testInnerVector.push_back(testString2);
    }

    for (unsigned int i = 0; i < numOfElements; i += 1) {
        DBusMessageIter subIter;
        dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
        dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &variantTypeIndex);
        DBusMessageIter subSubIter;
        dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "as", &subSubIter);

        DBusMessageIter innerArrayIter;
        dbus_message_iter_open_container(&subSubIter, DBUS_TYPE_ARRAY, "s", &innerArrayIter);
        for (unsigned int i = 0; i < numOfElements; i++) {
            dbus_message_iter_append_basic(&innerArrayIter, DBUS_TYPE_STRING, &testInnerVector[i]);
        }
        dbus_message_iter_close_container(&subSubIter, &innerArrayIter);

        dbus_message_iter_close_container(&subIter, &subSubIter);
        dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    }

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    TestedVariantType referenceVariant(testInnerVector);

    //Variant: structAlign + type-index(1) + variantSignature(4) + padding(3) + arrayLength(4) + stringLength(4) +
    //         string(37) + padding(3) + stringLength(4) + string(69) = 129
    EXPECT_EQ(129 + 7 + 129, scopedMessage.getBodyLength());
    for (unsigned int i = 0; i < numOfElements; i += 1) {
        TestedVariantType readVariant;
        inStream >> readVariant;

        std::vector<std::string> actualResult = readVariant.get<std::vector<std::string>>();

        bool variantsAreEqual = (referenceVariant == readVariant);
        EXPECT_TRUE(variantsAreEqual);
        EXPECT_EQ(testInnerVector, actualResult);
    }

}


TEST_F(InputStreamTest, ReadsVariantsWithVariants) {
    typedef CommonAPI::Variant<int8_t, uint64_t, CommonAPI::ByteBuffer> InnerVariantType;

    typedef CommonAPI::Variant<int32_t,
                               double,
                               std::string,
                               InnerVariantType>
            TestedVariantType;

    int8_t outerVariantTypeIndex = 1;
    int8_t innerVariant1TypeIndex = 1;
    int8_t innerVariant2TypeIndex = 3;

    const uint32_t byteBufferElementCount = numOfElements*10;

    CommonAPI::ByteBuffer innerVariant1Value;
    for (unsigned int i = 0; i < byteBufferElementCount; ++i) {
        innerVariant1Value.push_back((char) (i+40));
    }

    int8_t innerVariant2Value = -55;


    DBusMessageIter outerVariantStructIter;
    DBusMessageIter outerVariantActualIter;
    DBusMessageIter innerVariantStructIter;
    DBusMessageIter innerVariantActualIterator;
    DBusMessageIter innerArrayIter;


    //begin 1. outer variant
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &outerVariantStructIter);
    dbus_message_iter_append_basic(&outerVariantStructIter, DBUS_TYPE_BYTE, &outerVariantTypeIndex);
    dbus_message_iter_open_container(&outerVariantStructIter, DBUS_TYPE_VARIANT, "(yv)", &outerVariantActualIter);

    //begin inner variant
    dbus_message_iter_open_container(&outerVariantActualIter, DBUS_TYPE_STRUCT, NULL, &innerVariantStructIter);
    dbus_message_iter_append_basic(&innerVariantStructIter, DBUS_TYPE_BYTE, &innerVariant1TypeIndex);
    dbus_message_iter_open_container(&innerVariantStructIter, DBUS_TYPE_VARIANT, "ay", &innerVariantActualIterator);

    //begin inner variant content
    dbus_message_iter_open_container(&innerVariantActualIterator, DBUS_TYPE_ARRAY, "y", &innerArrayIter);
    for (unsigned int i = 0; i < byteBufferElementCount; i++) {
        dbus_message_iter_append_basic(&innerArrayIter, DBUS_TYPE_BYTE, &innerVariant1Value[i]);
    }
    dbus_message_iter_close_container(&innerVariantActualIterator, &innerArrayIter);
    //end inner variant content

    dbus_message_iter_close_container(&innerVariantStructIter, &innerVariantActualIterator);
    dbus_message_iter_close_container(&outerVariantActualIter, &innerVariantStructIter);
    //end inner variant

    dbus_message_iter_close_container(&outerVariantStructIter, &outerVariantActualIter);
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &outerVariantStructIter);
    //end 1. outer variant


    //begin 2. outer variant
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &outerVariantStructIter);
    dbus_message_iter_append_basic(&outerVariantStructIter, DBUS_TYPE_BYTE, &outerVariantTypeIndex);
    dbus_message_iter_open_container(&outerVariantStructIter, DBUS_TYPE_VARIANT, "(yv)", &outerVariantActualIter);

    //begin inner variant
    dbus_message_iter_open_container(&outerVariantActualIter, DBUS_TYPE_STRUCT, NULL, &innerVariantStructIter);
    dbus_message_iter_append_basic(&innerVariantStructIter, DBUS_TYPE_BYTE, &innerVariant2TypeIndex);
    dbus_message_iter_open_container(&innerVariantStructIter, DBUS_TYPE_VARIANT, "y", &innerVariantActualIterator);

    //begin inner variant content
    dbus_message_iter_append_basic(&innerVariantActualIterator, DBUS_TYPE_BYTE, &innerVariant2Value);
    //end inner variant content

    dbus_message_iter_close_container(&innerVariantStructIter, &innerVariantActualIterator);
    dbus_message_iter_close_container(&outerVariantActualIter, &innerVariantStructIter);
    //end inner variant

    dbus_message_iter_close_container(&outerVariantStructIter, &outerVariantActualIter);
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &outerVariantStructIter);
    //end 2. outer variant


    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    InnerVariantType referenceInnerVariant1(innerVariant1Value);
    InnerVariantType referenceInnerVariant2(innerVariant2Value);

    TestedVariantType referenceVariant1(referenceInnerVariant1);
    TestedVariantType referenceVariant2(referenceInnerVariant2);

    //Variant1: type-index(1) + varSigLen(1) + varSig(2) + struct-padding(4) + inner-type-index(1) + varLen(1) + varSig(3) +
    //          padding(3) + byteBufferLength(4) + byteBuffer(20) = 40
    //Variant2: type-index(1) + varSigLen(1) + varSig(2) + struct-padding(4) + inner-type-index(1) + varLen(1) + varSig(2) +
    //          byte(1) = 13
    // = 53
    EXPECT_EQ(53, scopedMessage.getBodyLength());

    TestedVariantType readVariant1;
    TestedVariantType readVariant2;
    inStream >> readVariant1;
    inStream >> readVariant2;
    EXPECT_EQ(referenceVariant1, readVariant1);
    EXPECT_EQ(referenceVariant2, readVariant2);

    InnerVariantType readInnerVariant1 = readVariant1.get<InnerVariantType>();
    InnerVariantType readInnerVariant2 = readVariant2.get<InnerVariantType>();
    EXPECT_EQ(referenceInnerVariant1, readInnerVariant1);
    EXPECT_EQ(referenceInnerVariant2, readInnerVariant2);

    CommonAPI::ByteBuffer readInnerValue1 = readInnerVariant1.get<CommonAPI::ByteBuffer>();
    int8_t readInnerValue2 = readInnerVariant2.get<int8_t>();
    EXPECT_EQ(innerVariant1Value, readInnerValue1);
    EXPECT_EQ(innerVariant2Value, readInnerValue2);
}


TEST_F(InputStreamTest, ReadsVariantsWithStructs) {
    typedef CommonAPI::Variant<int32_t,
                               double,
                               std::string,
                               bmw::test::TestSerializableStruct>
            TestedVariantType;

    int8_t variantTypeIndex = 1;

    bmw::test::TestSerializableStruct testStruct;
    testStruct.a = 15;
    testStruct.b = -32;
    testStruct.c = false;
    testStruct.d = "Hello all!";
    testStruct.e = 3.414;


    DBusMessageIter variantStructIter;
    DBusMessageIter variantActualIter;
    DBusMessageIter innerStructIter;


    //begin variant
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &variantStructIter);
    dbus_message_iter_append_basic(&variantStructIter, DBUS_TYPE_BYTE, &variantTypeIndex);
    dbus_message_iter_open_container(&variantStructIter, DBUS_TYPE_VARIANT, "(unbsd)", &variantActualIter);

    //begin variant content
    dbus_message_iter_open_container(&variantActualIter, DBUS_TYPE_STRUCT, NULL, &innerStructIter);

    dbus_bool_t dbusBool = 0;
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_UINT32, &testStruct.a);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_INT16, &testStruct.b);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_BOOLEAN, &dbusBool);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_STRING, &testStruct.d);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_DOUBLE, &testStruct.e);

    dbus_message_iter_close_container(&variantActualIter, &innerStructIter);
    //end variant content

    dbus_message_iter_close_container(&variantStructIter, &variantActualIter);
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &variantStructIter);
    //end variant

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);
    TestedVariantType referenceVariant(testStruct);

    //type-index(1) + varSigLen(1) + varSig(8) + struct-padding(6) + uint32(4) + int16(2) + padding(2) + bool(4) +
    //stringLen(4) + stringVal(11) + padding(5) + double(8) = 56
    EXPECT_EQ(56, scopedMessage.getBodyLength());

    TestedVariantType readVariant;
    inStream >> readVariant;

    bmw::test::TestSerializableStruct readStruct = readVariant.get<bmw::test::TestSerializableStruct>();
    EXPECT_EQ(testStruct.a, readStruct.a);
    EXPECT_EQ(testStruct.b, readStruct.b);
    EXPECT_EQ(testStruct.c, readStruct.c);
    EXPECT_EQ(testStruct.d, readStruct.d);
    EXPECT_EQ(testStruct.e, readStruct.e);
    EXPECT_EQ(testStruct, readStruct);
    EXPECT_EQ(referenceVariant, readVariant);
}


TEST_F(InputStreamTest, ReadsVariantsWithAnArrayOfStructs) {
    typedef CommonAPI::Variant<int32_t, double, std::vector<bmw::test::TestSerializableStruct>> TestedVariantType;

    bmw::test::TestSerializableStruct testStruct;
    testStruct.a = 15;
    testStruct.b = -32;
    testStruct.c = false;
    testStruct.d = "Hello all!";
    testStruct.e = 3.414;

    int8_t variantTypeIndex = 1;

    DBusMessageIter subIter;
    DBusMessageIter subSubIter;
    DBusMessageIter innerArrayIter;
    DBusMessageIter innerStructIter;

    //begin variant
    dbus_message_iter_open_container(&libdbusMessageWriteIter, DBUS_TYPE_STRUCT, NULL, &subIter);
    dbus_message_iter_append_basic(&subIter, DBUS_TYPE_BYTE, &variantTypeIndex);
    dbus_message_iter_open_container(&subIter, DBUS_TYPE_VARIANT, "a(unbsd)", &subSubIter);

    //begin array
    dbus_message_iter_open_container(&subSubIter, DBUS_TYPE_ARRAY, "(unbsd)", &innerArrayIter);

    //begin struct
    dbus_message_iter_open_container(&innerArrayIter, DBUS_TYPE_STRUCT, NULL, &innerStructIter);

    dbus_bool_t dbusBool = 0;
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_UINT32, &testStruct.a);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_INT16, &testStruct.b);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_BOOLEAN, &dbusBool);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_STRING, &testStruct.d);
    dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_DOUBLE, &testStruct.e);

    dbus_message_iter_close_container(&innerArrayIter, &innerStructIter);
    //end struct

    dbus_message_iter_close_container(&subSubIter, &innerArrayIter);
    //end array

    dbus_message_iter_close_container(&subIter, &subSubIter);
    dbus_message_iter_close_container(&libdbusMessageWriteIter, &subIter);
    //end variant

    CommonAPI::DBus::DBusMessage scopedMessage(libdbusMessage);
    CommonAPI::DBus::DBusInputStream inStream(scopedMessage);

    std::vector<bmw::test::TestSerializableStruct> referenceInnerVector;
    referenceInnerVector.push_back(testStruct);
    TestedVariantType referenceVariant(referenceInnerVector);

    //type-index(1) + varSigLen(1) + variantSig(9) + padding(1) + arrayLength(4) + uint32(4) + int16(2) + padding(2)
    //bool(4) + stringLength(4) + string(11) + padding(5) + double(8) = 56
    EXPECT_EQ(56, scopedMessage.getBodyLength());

    TestedVariantType readVariant;
    inStream >> readVariant;

    std::vector<bmw::test::TestSerializableStruct> actualResult = readVariant.get<std::vector<bmw::test::TestSerializableStruct>>();
    bmw::test::TestSerializableStruct readStruct = actualResult[0];

    EXPECT_EQ(testStruct.a, readStruct.a);
    EXPECT_EQ(testStruct.b, readStruct.b);
    EXPECT_EQ(testStruct.c, readStruct.c);
    EXPECT_EQ(testStruct.d, readStruct.d);
    EXPECT_EQ(testStruct.e, readStruct.e);
    EXPECT_EQ(testStruct, readStruct);
    EXPECT_EQ(referenceInnerVector, actualResult);
    EXPECT_EQ(referenceVariant, readVariant);
}


int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
