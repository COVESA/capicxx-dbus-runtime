/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <dbus/dbus.h>
#include <gtest/gtest.h>

#include <CommonAPI/DBus/DBusMessage.h>
#include <CommonAPI/DBus/DBusOutputStream.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/SerializableVariant.h>

#include "commonapi/tests/DerivedTypeCollection.h"

class OutputStreamTest: public ::testing::Test {
protected:
    size_t numOfElements;
    CommonAPI::DBus::DBusMessage message;
    const char* busName;
    const char* objectPath;
    const char* interfaceName;
    const char* methodName;

    void SetUp() {
        numOfElements = 10;
        busName = "no.bus.here";
        objectPath = "/no/object/here";
        interfaceName = "no.interface.here";
        methodName = "noMethodHere";
    }

    void TearDown() {
    }
};

TEST_F(OutputStreamTest, CanBeConstructed) {
    CommonAPI::DBus::DBusOutputStream outStream(message);
}

TEST_F(OutputStreamTest, WritesBytes) {
    const char signature[] = "yyyyyyyyyy";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements);
    uint8_t val1 = 0xff;
    uint8_t val2 = 0x00;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    uint8_t verifyVal1;
    uint8_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesBools) {
    const char signature[] = "bbbbbbbbbb";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 4);
    bool val1 = TRUE;
    bool val2 = FALSE;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*4, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    bool verifyVal1;
    bool verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesUInt16) {
    const char signature[] = "qqqqqqqqqq";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 2);
    uint16_t val1 = 0x0000;
    uint16_t val2 = 0xffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*2, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    uint16_t verifyVal1;
    uint16_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesInt16) {
    const char signature[] = "nnnnnnnnnn";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 2);
    int16_t val1 = 0x7fff;
    int16_t val2 = 0xffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*2, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    int16_t verifyVal1;
    int16_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesUInt32) {
    const char signature[] = "uuuuuuuuuu";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 4);
    uint32_t val1 = 0x00000000;
    uint32_t val2 = 0xffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*4, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    uint32_t verifyVal1;
    uint32_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesInt32) {
    const char signature[] = "iiiiiiiiii";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 4);
    int32_t val1 = 0x7fffffff;
    int32_t val2 = 0xffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*4, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    int32_t verifyVal1;
    int32_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesUInt64) {
    const char signature[] = "tttttttttt";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 8);
    uint64_t val1 = 0x0000000000000000;
    uint64_t val2 = 0xffffffffffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*8, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    uint64_t verifyVal1;
    uint64_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesInt64) {
    const char signature[] = "xxxxxxxxxx";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 8);
    int64_t val1 = 0x7fffffffffffffff;
    int64_t val2 = 0xffffffffffffffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*8, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    int64_t verifyVal1;
    int64_t verifyVal2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesDouble) {
    const char signature[] = "dddddddddd";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    outStream.reserveMemory(numOfElements * 8);
    double val1 = 13.37;
    double val2 = 3.414;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        outStream << val1;
        outStream << val2;
    }
    outStream.flush();

    EXPECT_EQ(numOfElements*8, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    double verifyVal1;
    double verifyVal2;
    std::string verifySignature;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        inStream >> verifyVal1;
        EXPECT_EQ(val1, verifyVal1);

        inStream >> verifyVal2;
        EXPECT_EQ(val2, verifyVal2);
    }
}

TEST_F(OutputStreamTest, WritesStrings) {
    const char signature[] = "sss";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    std::string val1 = "hai";
    std::string val2 = "ciao";
    std::string val3 = "salut";

    //sizes of the strings + terminating null-bytes (each 1 byte) + length-fields (each 4 bytes)
    outStream.reserveMemory(val1.size() + val2.size() + val3.size() + 3 + 3 * 4);
    outStream << val1 << val2 << val3;
    outStream.flush();

    //Length fields + actual strings + terminating '\0's + 3(padding)
    EXPECT_EQ(3*4 + 3 + 4 + 5 + 3 + 3, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    std::string verifyVal1;
    std::string verifyVal2;
    std::string verifyVal3;
    std::string verifySignature;

    inStream >> verifyVal1;
    inStream >> verifyVal2;
    inStream >> verifyVal3;

    EXPECT_EQ(val1, verifyVal1);
    EXPECT_EQ(val2, verifyVal2);
    EXPECT_EQ(val3, verifyVal3);
}

namespace bmw {
namespace test {

struct myStruct: CommonAPI::SerializableStruct {
    ~myStruct();

    uint32_t a;
    int16_t b;
    bool c;
    std::string d;
    double e;

    virtual void readFromInputStream(CommonAPI::InputStream& inputMessageStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const;
};

myStruct::~myStruct() {
}

void myStruct::readFromInputStream(CommonAPI::InputStream& inputMessageStream) {
    inputMessageStream >> a >> b >> c >> d >> e;
}

void myStruct::writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const {
    outputMessageStream << a << b << c << d << e;
}

} //namespace test
} //namespace bmw

TEST_F(OutputStreamTest, WritesStructs) {
    const char signature[] = "(unbsd)";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    bmw::test::myStruct testStruct;
    testStruct.a = 15;
    testStruct.b = -32;
    testStruct.c = FALSE;
    testStruct.d = "Hello all";
    testStruct.e = 3.414;

    // 40(byte length of struct) = 4(uint32_t) + 2(int16_t) + 2(padding) + 4(bool) + 4(strlength)
    //                           + 9(string) + 1(terminating '\0' of string) + 6(padding) + 8 (double)
    outStream.reserveMemory(40);
    outStream << testStruct;
    outStream.flush();

    EXPECT_EQ(40, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    bmw::test::myStruct verifyStruct;
    inStream >> verifyStruct;

    EXPECT_EQ(testStruct.a, verifyStruct.a);
    EXPECT_EQ(testStruct.b, verifyStruct.b);
    EXPECT_EQ(testStruct.c, verifyStruct.c);
    EXPECT_EQ(testStruct.d, verifyStruct.d);
    EXPECT_EQ(testStruct.e, verifyStruct.e);
}

TEST_F(OutputStreamTest, WritesArrays) {
    const char signature[] = "ai";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    std::vector<int32_t> testVector;
    int32_t val1 = 0xffffffff;
    int32_t val2 = 0x7fffffff;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        testVector.push_back(val1);
        testVector.push_back(val2);
    }

    outStream.reserveMemory(numOfElements * 4 + 4);
    outStream << testVector;
    outStream.flush();

    EXPECT_EQ(numOfElements*4 + 4, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
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

TEST_F(OutputStreamTest, WritesArraysOfStrings) {
    const char signature[] = "as";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    std::vector<std::string> testVector;
    std::string val1 = "Hai";
    std::string val2 = "Ciao";
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        testVector.push_back(val1);
        testVector.push_back(val2);
    }

    // 101 = 4(lengthFieldOfArray) +
    //       4*(4(lengthField1) + 4(string1 mit \0-byte) + 4(lengthField2) + 5(string2 mit \0-byte) + 3(paddingTo4)) +
    //         (4(lengthField1) + 4(string1 mit \0-byte) + 4(lengthField2) + 5(string2 mit \0-byte))
    size_t vectorLength = 101;
    outStream.reserveMemory(vectorLength);
    outStream << testVector;
    outStream.flush();

    EXPECT_EQ(vectorLength, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    std::vector<std::string> verifyVector;
    inStream >> verifyVector;

    std::string res1;
    std::string res2;
    for (unsigned int i = 0; i < numOfElements; i += 2) {
        res1 = verifyVector[i];
        EXPECT_EQ(val1, res1);
        res2 = verifyVector[i + 1];
        EXPECT_EQ(val2, res2);
    }
}

TEST_F(OutputStreamTest, WritesArraysInArrays) {
    const char signature[] = "aai";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

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

    outStream.reserveMemory(numOfElements * numOfElements * 4 + numOfElements * 4 + 4);
    outStream << testVector;
    outStream.flush();

    EXPECT_EQ(numOfElements*numOfElements*4 + numOfElements*4 + 4, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
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

typedef CommonAPI::Variant<int8_t, uint32_t, double, std::string> BasicTypeVariant;

TEST_F(OutputStreamTest, WritesArraysOfVariants) {
    numOfElements = 4;
    const char signature[] = "(yv)(yv)(yv)(yv)";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    int8_t fromInt8Value = 7;
    uint32_t fromUInt32Value = 42;
    double fromDoubleValue = 13.37;
    std::string fromStringValue = "Hai :)";
    BasicTypeVariant referenceInt8Variant(fromInt8Value);
    BasicTypeVariant referenceUint32Variant(fromUInt32Value);
    BasicTypeVariant referenceDoubleVariant(fromDoubleValue);
    BasicTypeVariant referenceStringVariant(fromStringValue);

    std::vector<BasicTypeVariant> testVector;

    testVector.push_back(referenceInt8Variant);
    testVector.push_back(fromUInt32Value);
    testVector.push_back(fromDoubleValue);
    testVector.push_back(fromStringValue);

    // 4(length field) + 4(padding) + (1(variant type byte) + 3(signature length + content + terminating null) + 1(int8_t))
    //                 + 3(padding) + (1(variant type byte) + 3(signature length + content + terminating null) + 4(uint32_t))
    //                              + (1(variant type byte) + 3(signature length + content + terminating null) + 8(uint32_t))
    //                 + 4(padding) + (1(variant type byte) + 3(signature length + content + terminating null) + 4(string length field) + 7(string))
    // = 55
    outStream.reserveMemory(55);
    outStream << testVector;
    outStream.flush();

    EXPECT_EQ(55, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    std::vector<BasicTypeVariant> verifyVector;
    inStream >> verifyVector;

    BasicTypeVariant resultInt8Variant = verifyVector[0];
    BasicTypeVariant resultUint32Variant = verifyVector[1];
    BasicTypeVariant resultDoubleVariant = verifyVector[2];
    BasicTypeVariant resultStringVariant = verifyVector[3];

    EXPECT_EQ(referenceInt8Variant, resultInt8Variant);
    EXPECT_EQ(referenceUint32Variant, resultUint32Variant);
    EXPECT_EQ(referenceDoubleVariant, resultDoubleVariant);
    EXPECT_EQ(referenceStringVariant, resultStringVariant);
}

namespace com {
namespace bmw {
namespace test {

struct TestStruct: CommonAPI::SerializableStruct {
    TestStruct();
    TestStruct(int32_t v1, double v2, double v3, std::string v4);
    ~TestStruct();

    int32_t val1;
    double val2;
    double val3;
    std::string val4;

    virtual void readFromInputStream(CommonAPI::InputStream& inputMessageStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const;
};

typedef std::vector<TestStruct> TestStructList;

TestStruct::TestStruct() :
                val1(0), val2(0), val3(0), val4("") {
}

TestStruct::TestStruct(int32_t v1, double v2, double v3, std::string v4) :
                val1(v1), val2(v2), val3(v3), val4(v4) {
}

TestStruct::~TestStruct() {
}

void TestStruct::readFromInputStream(CommonAPI::InputStream& inputMessageStream) {
    inputMessageStream >> val1 >> val2 >> val3 >> val4;
}

void TestStruct::writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const {
    outputMessageStream << val1 << val2 << val3 << val4;
}

} // namespace test
} // namespace bmw
} // namespace com

TEST_F(OutputStreamTest, WritesTestStructs) {
    const char signature[] = "(idds)";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    com::bmw::test::TestStruct testStruct(1, 12.6, 1e40, "XXXXXXXXXXXXXXXXXXXX");

    //4(int32_t) + 4(padding) + 8(double) + 8(double) + 4(str_length) + 20(string) + 1(null-termination)
    uint32_t expectedSize = 49;
    outStream.reserveMemory(expectedSize);
    outStream << testStruct;
    outStream.flush();

    EXPECT_EQ(expectedSize, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    com::bmw::test::TestStruct verifyStruct;
    inStream >> verifyStruct;

    EXPECT_EQ(testStruct.val1, verifyStruct.val1);
    EXPECT_EQ(testStruct.val2, verifyStruct.val2);
    EXPECT_EQ(testStruct.val3, verifyStruct.val3);
    EXPECT_EQ(testStruct.val4, verifyStruct.val4);
}

TEST_F(OutputStreamTest, WritesTestStructLists) {
    const char signature[] = "a(idds)";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    com::bmw::test::TestStructList testList;
    for (unsigned int i = 0; i < numOfElements; i++) {
        testList.emplace_back(1, 12.6, 1e40, "XXXXXXXXXXXXXXXXXXXX");
    }

    //struct size: 49 = 4(int32_t) + 4(padding) + 8(double) + 8(double) + 4(str_length) + 20(string) + 1(null-termination)
    //array size:  4(array_length) + 4(struct_padding) + (numElements-1)*(49(struct) + 7(struct_padding)) + 49(struct)
    uint32_t expectedSize = 8 + (numOfElements - 1) * (49 + 7) + 49;
    outStream.reserveMemory(expectedSize);
    outStream << testList;
    outStream.flush();

    EXPECT_EQ(expectedSize, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    com::bmw::test::TestStructList verifyList;
    inStream >> verifyList;

    EXPECT_EQ(numOfElements, verifyList.size());
}

namespace com {
namespace bmw {
namespace test {

struct ArrayStruct: CommonAPI::SerializableStruct {
    ArrayStruct();
    ArrayStruct(std::vector<int64_t> v1,
                std::vector<std::string> v2,
                std::vector<double> v3,
                std::vector<std::string> v4,
                std::vector<uint16_t> v5);
    ~ArrayStruct();

    std::vector<int64_t> val1;
    std::vector<std::string> val2;
    std::vector<double> val3;
    std::vector<std::string> val4;
    std::vector<uint16_t> val5;

    virtual void readFromInputStream(CommonAPI::InputStream& inputMessageStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const;
};

typedef std::vector<TestStruct> TestStructList;

ArrayStruct::ArrayStruct() {
}

ArrayStruct::ArrayStruct(std::vector<int64_t> v1,
                         std::vector<std::string> v2,
                         std::vector<double> v3,
                         std::vector<std::string> v4,
                         std::vector<uint16_t> v5) :
                val1(v1), val2(v2), val3(v3), val4(v4), val5(v5) {
}

ArrayStruct::~ArrayStruct() {
}

void ArrayStruct::readFromInputStream(CommonAPI::InputStream& inputMessageStream) {
    inputMessageStream >> val1 >> val2 >> val3 >> val4 >> val5;
}

void ArrayStruct::writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const {
    outputMessageStream << val1 << val2 << val3 << val4 << val5;
}

} // namespace test
} // namespace bmw
} // namespace com

TEST_F(OutputStreamTest, WritesStructsOfArraysWithSthBefore) {
    const char signature[] = "(axasadasaq)";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    com::bmw::test::ArrayStruct arrayStruct;
    for (unsigned int i = 0; i < numOfElements; i++) {
        arrayStruct.val1.push_back(i * 50);
        arrayStruct.val2.push_back("Hai");
        arrayStruct.val3.push_back(3.414);
        arrayStruct.val4.push_back("Ciao");
        arrayStruct.val5.push_back(i * 5);
    }
    uint16_t frontValue = 0;

    // 2(uint16) + 6(padding)                                             --> 8
    // 4(LengthField) + 4(padding) + 10 * 8(int64)                        --> 88  --> 96
    // 4(LengthField) + 10 * (4(LengthField) + 4("Hai\0"))                --> 84  --> 180
    // 4(LengthField) + 10 * 8(double)                                    --> 84  --> 264
    // 4(LengthField) + 10 * (4(LengthField) + 5("Ciao\0") + 3(padding))  --> 124 --> 388
    // 4(LengthField) + 10 * 2(uint16)                                    --> 24  --> 412
    size_t structLength = 412;
    outStream.reserveMemory(structLength);
    outStream << frontValue << arrayStruct;
    outStream.flush();

    EXPECT_EQ(structLength, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);
    com::bmw::test::ArrayStruct verifyStruct;

    uint16_t frontVerification;
    inStream >> frontVerification >> verifyStruct;

    EXPECT_EQ(frontValue, frontVerification);

    int64_t res1;
    std::string res2;
    double res3;
    std::string res4;
    uint16_t res5;

    for (unsigned int i = 0; i < numOfElements; i++) {
        res1 = verifyStruct.val1[i];
        res2 = verifyStruct.val2[i];
        res3 = verifyStruct.val3[i];
        res4 = verifyStruct.val4[i];
        res5 = verifyStruct.val5[i];

        EXPECT_EQ(i*50, res1);
        EXPECT_EQ(std::string("Hai"), res2);
        EXPECT_EQ(3.414, res3);
        EXPECT_EQ(std::string("Ciao"), res4);
        EXPECT_EQ(i*5, res5);
    }
}

TEST_F(OutputStreamTest, WritesEnumKeyedMaps) {
    commonapi::tests::DerivedTypeCollection::TestEnumMap testEnumMap;

    commonapi::tests::DerivedTypeCollection::TestEnum key1 =
                    commonapi::tests::DerivedTypeCollection::TestEnum::E_NOT_USED;
    commonapi::tests::DerivedTypeCollection::TestEnum key2 = commonapi::tests::DerivedTypeCollection::TestEnum::E_OK;
    commonapi::tests::DerivedTypeCollection::TestEnum key3 =
                    commonapi::tests::DerivedTypeCollection::TestEnum::E_OUT_OF_RANGE;
    commonapi::tests::DerivedTypeCollection::TestEnum key4 =
                    commonapi::tests::DerivedTypeCollection::TestEnum::E_UNKNOWN;
    std::string val = "Hai";

    testEnumMap.insert( {key1, val});
    testEnumMap.insert( {key2, val});
    testEnumMap.insert( {key3, val});
    testEnumMap.insert( {key4, val});

    const char signature[] = "a{is}";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    CommonAPI::DBus::DBusOutputStream outStream(message);

    //array length (4) + struct-padding (4) +
    //      3 * (sizeof(int) + sizeof(string) + struct-padding) + (sizeof(int) + sizeof(string))
    size_t sizeOfBody = 4 + 4 + 3 * (4 + 8 + 4) + (4 + 8);
    outStream.reserveMemory(sizeOfBody);
    outStream << testEnumMap;
    outStream.flush();

    EXPECT_EQ(sizeOfBody, message.getBodyLength());

    CommonAPI::DBus::DBusInputStream inStream(message);

    commonapi::tests::DerivedTypeCollection::TestEnumMap verificationMap;
    inStream >> verificationMap;

    for (auto it = verificationMap.begin(); it != verificationMap.end(); ++it) {
        ASSERT_EQ(it->second, testEnumMap[it->first]);
    }
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif