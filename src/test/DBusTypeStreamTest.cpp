/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <unordered_map>
#include <vector>

#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/SerializableVariant.h>
#include <CommonAPI/types.h>
#include <CommonAPI/ByteBuffer.h>

#include <CommonAPI/DBus/DBusOutputStream.h>

#include <type_traits>





//##############################################################################################################



class TypeOutputStreamTest: public ::testing::Test {
  protected:

    void SetUp() {
    }

    void TearDown() {
    }
};



TEST_F(TypeOutputStreamTest, CreatesBoolSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<bool>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("b") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesInt8Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<int8_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt16Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<int16_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("n") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt32Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<int32_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("i") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt64Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<int64_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("x") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesUInt8Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<uint8_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt16Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<uint16_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("q") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt32Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<uint32_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("u") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt64Signature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<uint64_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("t") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesFloatSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<float>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesDoubleSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<double>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesStringSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::string>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("s") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesByteBufferSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<CommonAPI::ByteBuffer>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("ay") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesVersionSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<CommonAPI::Version>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("(uu)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVariantWithBasicTypesSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<CommonAPI::Variant<int, double, std::string>>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("v") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVariantWithVariantSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<CommonAPI::Variant<int, CommonAPI::Variant<int, double, std::string>, CommonAPI::Variant<int, CommonAPI::Variant<int, double, std::string>, std::string>, std::string>>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("v") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfStringsSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::vector<std::string> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("as") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVersionsSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::vector<CommonAPI::Version> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("a(uu)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVariantsSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::vector<CommonAPI::Variant<int, double, std::string>> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("av") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVectorOfStringsSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::vector<std::vector<std::string>> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("aas") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesMapOfUInt16ToStringSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::unordered_map<uint16_t, std::string>>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("a{qs}") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfMapsOfUInt16ToStringSignature) {
    CommonAPI::DBus::DBusTypeOutputStream typeStream;
    CommonAPI::TypeWriter<std::vector<std::unordered_map<uint16_t, std::string>> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("aa{qs}") == 0);
}





int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
