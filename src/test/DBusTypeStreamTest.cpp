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

#include "commonapi/tests/DerivedTypeCollection.h"

#include <type_traits>


struct TestStructWithVariant: public CommonAPI::SerializableStruct {
    TestStructWithVariant(): uint16Val_(42), stringVal_("Hai!"), variantVal_(13.37) {

    }

    virtual ~TestStructWithVariant() {}

    virtual void readFromInputStream(CommonAPI::InputStream& inputStream) {
    }

    virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const {
    }

    static void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
        CommonAPI::TypeWriter<uint16_t>::writeType(typeOutputStream);
        CommonAPI::TypeWriter<std::string>::writeType(typeOutputStream);
        CommonAPI::TypeWriter<CommonAPI::Variant<uint32_t, double, std::vector<std::string>>>::writeType(typeOutputStream);
    }

  private:
    uint16_t uint16Val_;
    std::string stringVal_;
    CommonAPI::Variant<uint32_t, double, std::vector<std::string>> variantVal_;
};


typedef std::vector<commonapi::tests::DerivedTypeCollection::TestEnum> TestEnumList;


class TypeOutputStreamTest: public ::testing::Test {
  protected:

    void SetUp() {
    }

    void TearDown() {
    }

    CommonAPI::DBus::DBusTypeOutputStream typeStream_;
};



TEST_F(TypeOutputStreamTest, CreatesBasicTypeSignatures) {
    CommonAPI::TypeWriter<bool>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("b") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesInt8Signature) {
    CommonAPI::TypeWriter<int8_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt16Signature) {
    CommonAPI::TypeWriter<int16_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("n") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt32Signature) {
    CommonAPI::TypeWriter<int32_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("i") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesInt64Signature) {
    CommonAPI::TypeWriter<int64_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("x") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesUInt8Signature) {
    CommonAPI::TypeWriter<uint8_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt16Signature) {
    CommonAPI::TypeWriter<uint16_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("q") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt32Signature) {
    CommonAPI::TypeWriter<uint32_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("u") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesUInt64Signature) {
    CommonAPI::TypeWriter<uint64_t>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("t") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesFloatSignature) {
    CommonAPI::TypeWriter<float>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}
TEST_F(TypeOutputStreamTest, CreatesDoubleSignature) {
    CommonAPI::TypeWriter<double>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesStringSignature) {
    CommonAPI::TypeWriter<std::string>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("s") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesByteBufferSignature) {
    CommonAPI::TypeWriter<CommonAPI::ByteBuffer>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("ay") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVersionSignature) {
    CommonAPI::TypeWriter<CommonAPI::Version>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(uu)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesInt32EnumSignature) {
    CommonAPI::TypeWriter<commonapi::tests::DerivedTypeCollection::TestEnum>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("i") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVariantWithBasicTypesSignature) {
    CommonAPI::TypeWriter<CommonAPI::Variant<int, double, std::string>>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(yv)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVariantWithVariantSignature) {
    CommonAPI::TypeWriter<CommonAPI::Variant<int, CommonAPI::Variant<int, double, std::string>, CommonAPI::Variant<int, CommonAPI::Variant<int, double, std::string>, std::string>, std::string>>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(yv)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfStringsSignature) {
    CommonAPI::TypeWriter<std::vector<std::string> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("as") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfInt32EnumsSignature) {
    CommonAPI::TypeWriter<TestEnumList>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("ai") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVersionsSignature) {
    CommonAPI::TypeWriter<std::vector<CommonAPI::Version> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("a(uu)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVariantsSignature) {
    CommonAPI::TypeWriter<std::vector<CommonAPI::Variant<int, double, std::string>> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("a(yv)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVectorOfStringsSignature) {
    CommonAPI::TypeWriter<std::vector<std::vector<std::string>> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("aas") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfVectorOfVariantsSignature) {
    CommonAPI::TypeWriter<std::vector<std::vector<CommonAPI::Variant<int, double, std::string>>> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("aa(yv)") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesMapOfUInt16ToStringSignature) {
    CommonAPI::TypeWriter<std::unordered_map<uint16_t, std::string>>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("a{qs}") == 0);
}

TEST_F(TypeOutputStreamTest, CreatesVectorOfMapsOfUInt16ToStringSignature) {
    CommonAPI::TypeWriter<std::vector<std::unordered_map<uint16_t, std::string>> >::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("aa{qs}") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfTestStructCorrectly) {
    CommonAPI::TypeWriter<TestStructWithVariant>::writeType(typeStream_);
    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(qs(yv))") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfUInt32TypeVariantsCorrectly) {
    uint32_t fromUInt = 42;
    CommonAPI::Variant<uint32_t, double, uint16_t> myVariant(fromUInt);

    myVariant.writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("u") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfStringTypeVariantsCorrectly) {
    std::string fromString = "Hai!";
    CommonAPI::Variant<uint32_t, double, std::string> myVariant(fromString);

    myVariant.writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("s") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfVectorTypeVariantsCorrectly) {
    std::vector<std::string> fromStringVector;
    CommonAPI::Variant<uint32_t, double, std::vector<std::string>> myVariant(fromStringVector);

    myVariant.writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("as") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfTestStructTypeVariantsCorrectly) {
    TestStructWithVariant fromTestStruct;
    CommonAPI::Variant<uint32_t, double, TestStructWithVariant> myVariant(fromTestStruct);

    myVariant.writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(qs(yv))") == 0);
}

TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericUInt32TypeVariantsCorrectly) {
    uint32_t fromUInt = 42;
    CommonAPI::Variant<uint32_t, double, uint16_t> myVariant(fromUInt);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("u") == 0);
}

TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericInt32EnumTypeVariantsCorrectly) {
    commonapi::tests::DerivedTypeCollection::TestEnum fromInt32Enum = commonapi::tests::DerivedTypeCollection::TestEnum::E_OK;
    CommonAPI::Variant<uint32_t, double, commonapi::tests::DerivedTypeCollection::TestEnum> myVariant(fromInt32Enum);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("i") == 0);
}

TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericStringTypeVariantsCorrectly) {
    std::string fromString = "Hai!";
    CommonAPI::Variant<uint32_t, double, std::string> myVariant(fromString);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("s") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericVectorTypeVariantsCorrectly) {
    std::vector<std::string> fromStringVector;
    CommonAPI::Variant<uint32_t, double, std::vector<std::string>> myVariant(fromStringVector);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("as") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericVectorOfInt32EnumTypeVariantsCorrectly) {
    TestEnumList fromEnumVector;
    CommonAPI::Variant<uint32_t, double, TestEnumList> myVariant(fromEnumVector);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("ai") == 0);
}


TEST_F(TypeOutputStreamTest, ParsesSignatureOfGenericTestStructTypeVariantsCorrectly) {
    TestStructWithVariant fromTestStruct;
    CommonAPI::Variant<uint32_t, double, TestStructWithVariant> myVariant(fromTestStruct);
    CommonAPI::SerializableVariant* genericVariant = &myVariant;

    genericVariant->writeToTypeOutputStream(typeStream_);

    std::string signature = typeStream_.retrieveSignature();
    ASSERT_TRUE(signature.compare("(qs(yv))") == 0);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif