/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>
#include <CommonAPI/DBus/DBusOutputStream.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/SerializableVariant.h>

using namespace CommonAPI;

class VariantOutputStreamTest: public ::testing::Test {
  protected:
    size_t numOfElements;
    CommonAPI::DBus::DBusMessage message;
    const char* busName;
    const char* objectPath;
    const char* interfaceName;
    const char* methodName;
    const char* signature;

    void SetUp() {
      numOfElements = 10;
      busName = "no.bus.here";
      objectPath = "/no/object/here";
      interfaceName = "no.interface.here";
      methodName = "noMethodHere";
      signature = "yyyyyyyyyy";
    }

    void TearDown() {
    }

};

typedef Variant<int,bool> InnerVar;

struct MyStruct: CommonAPI::SerializableStruct {
    ~MyStruct();

    uint32_t a;
    InnerVar b;
    bool c;
    std::string d;
    double e;

    virtual void readFromInputStream(CommonAPI::InputStream& inputMessageStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const;
    static inline void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
        typeOutputStream.writeUInt32Type();
        typeOutputStream.writeVariantType();
        typeOutputStream.writeBoolType();
        typeOutputStream.writeStringType();
        typeOutputStream.writeDoubleType();
    }
};

MyStruct::~MyStruct() {
}

void MyStruct::readFromInputStream(CommonAPI::InputStream& inputMessageStream) {
  inputMessageStream >> a >> b >> c >> d >> e;
}

void MyStruct::writeToOutputStream(CommonAPI::OutputStream& outputMessageStream) const {
  outputMessageStream << a << b << c << d << e;
}

bool operator==(const MyStruct& lhs, const MyStruct& rhs) {
    if (&lhs == &rhs)
        return true;

    return
        lhs.a == rhs.a &&
        lhs.b == rhs.b &&
        lhs.c == rhs.c &&
        lhs.d == rhs.d &&
        lhs.e == rhs.e
    ;
}

TEST_F(VariantOutputStreamTest, CanBeCalled) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);
}

TEST_F(VariantOutputStreamTest, CanWriteVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    int fromInt = 14132;
    Variant<int, bool> inVariant(fromInt);
    Variant<int, bool> outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<int>());
    EXPECT_EQ(inVariant.get<int>(), outVariant.get<int>());
    EXPECT_TRUE(inVariant == outVariant);
}

TEST_F(VariantOutputStreamTest, CanWriteVariantInVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    int fromInt = 14132;
    Variant<int, bool> nestedVariant(fromInt);

    Variant<InnerVar, std::string, float> inVariant(nestedVariant);

    Variant<InnerVar, std::string, float> outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<InnerVar>());
    EXPECT_EQ(inVariant.get<InnerVar>(), outVariant.get<InnerVar>());
    EXPECT_TRUE(inVariant == outVariant);
}

TEST_F(VariantOutputStreamTest, CanWriteVariantInStruct) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    int fromInt = 14132;
    Variant<int, bool> nestedVariant(fromInt);

    MyStruct inStruct;
    inStruct.b = nestedVariant;

    MyStruct outStruct;

    outputStream << inStruct;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outStruct;

    EXPECT_TRUE(outStruct.b.isType<int>());
    EXPECT_EQ(outStruct.b.get<int>(), inStruct.b.get<int>());
    EXPECT_TRUE(inStruct.b == outStruct.b);
}

TEST_F(VariantOutputStreamTest, CanWriteVariantInArray) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    int fromInt = 14132;
    std::vector<InnerVar> inVector;
    std::vector<InnerVar> outVector;

    for (unsigned int i = 0; i < numOfElements; i++) {
        inVector.push_back(InnerVar(fromInt));
    }

    outputStream << inVector;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVector;

    EXPECT_TRUE(outVector[0].isType<int>());
    EXPECT_EQ(inVector[0].get<int>(), outVector[0].get<int>());
    EXPECT_EQ(numOfElements, outVector.size());
    EXPECT_TRUE(inVector[0] == outVector[0]);
}

TEST_F(VariantOutputStreamTest, CanWriteArrayInVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    typedef std::vector<int> IntVector;
    typedef Variant<IntVector, std::string> VectorVariant;

    std::vector<int> inVector;
    int fromInt = 14132;
    for (unsigned int i = 0; i < numOfElements; i++) {
        inVector.push_back(fromInt);
    }


    VectorVariant inVariant(inVector);
    VectorVariant outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<IntVector>());
    EXPECT_EQ(inVariant.get<IntVector>(), outVariant.get<IntVector>());
    EXPECT_TRUE(inVariant == outVariant);
}

TEST_F(VariantOutputStreamTest, CanWriteStructInVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    typedef Variant<MyStruct, std::string> StructVariant;

    MyStruct str;
    int fromInt = 14132;
    str.a = fromInt;

    StructVariant inVariant(str);
    StructVariant outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<MyStruct>());
    MyStruct iStr = inVariant.get<MyStruct>();
    MyStruct oStr = outVariant.get<MyStruct>();
    EXPECT_EQ(iStr, oStr);
    EXPECT_TRUE(inVariant == outVariant);
}

TEST_F(VariantOutputStreamTest, CanWriteVariantInStructInVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    typedef Variant<MyStruct, std::string> StructVariant;

    MyStruct str;
    int fromInt = 14132;
    str.b = InnerVar(fromInt);

    StructVariant inVariant(str);
    StructVariant outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<MyStruct>());
    EXPECT_EQ(inVariant.get<MyStruct>(), outVariant.get<MyStruct>());
    EXPECT_TRUE(inVariant.get<MyStruct>().b == outVariant.get<MyStruct>().b);
    EXPECT_TRUE(inVariant.get<MyStruct>().b.get<int>() == outVariant.get<MyStruct>().b.get<int>());
    EXPECT_TRUE(inVariant == outVariant);
}

TEST_F(VariantOutputStreamTest, CanWriteVariantInArrayInVariant) {
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);

    typedef std::vector<InnerVar> VarVector;
    typedef Variant<VarVector, std::string> ArrayVariant;

    VarVector inVector;
    int fromInt = 14132;
    for (unsigned int i = 0; i < numOfElements; i++) {
        inVector.push_back(InnerVar(fromInt));
    }

    ArrayVariant inVariant(inVector);
    ArrayVariant outVariant;

    outputStream << inVariant;
    outputStream.flush();

    DBus::DBusInputStream inputStream(message);

    inputStream >> outVariant;

    EXPECT_TRUE(outVariant.isType<VarVector>());
    EXPECT_EQ(inVariant.get<VarVector>(), outVariant.get<VarVector>());
    EXPECT_TRUE(inVariant.get<VarVector>()[0] == outVariant.get<VarVector>()[0]);
    EXPECT_TRUE(inVariant.get<VarVector>()[0].get<int>() == outVariant.get<VarVector>()[0].get<int>());
    EXPECT_TRUE(inVariant == outVariant);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
