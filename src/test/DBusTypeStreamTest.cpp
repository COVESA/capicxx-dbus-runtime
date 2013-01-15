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

#include <type_traits>



class TypeStream {
  public:
    virtual ~TypeStream() {}


    virtual void writeBoolType() = 0;

    virtual void writeInt8Type() = 0;
    virtual void writeInt16Type() = 0;
    virtual void writeInt32Type() = 0;
    virtual void writeInt64Type() = 0;

    virtual void writeUInt8Type() = 0;
    virtual void writeUInt16Type() = 0;
    virtual void writeUInt32Type() = 0;
    virtual void writeUInt64Type() = 0;


    virtual void writeInt8EnumType() = 0;
    virtual void writeInt16EnumType() = 0;
    virtual void writeInt32EnumType() = 0;
    virtual void writeInt64EnumType() = 0;

    virtual void writeUInt8EnumType() = 0;
    virtual void writeUInt16EnumType() = 0;
    virtual void writeUInt32EnumType() = 0;
    virtual void writeUInt64EnumType() = 0;


    virtual void writeFloatType() = 0;
    virtual void writeDoubleType() = 0;

    virtual void writeStringType() = 0;
    virtual void writeByteBufferType() = 0;
    virtual void writeVersionType() = 0;

    virtual void writeVectorType() = 0;

    virtual void beginWriteMapType() = 0;
    virtual void endWriteMapType() = 0;

    virtual void beginWriteStructType() = 0;
    virtual void endWriteStructType() = 0;

    virtual void writeVariantType() = 0;

    virtual std::string retrieveSignature() = 0;
};


//##############################################################################################################


class DBusTypeStream: public TypeStream {
  public:
    DBusTypeStream(): signature_("") {

    }
    virtual ~DBusTypeStream() {}


    inline virtual void writeBoolType() {
        signature_.append("b");
    }

    inline virtual void writeInt8Type()  {
        signature_.append("y");
    }
    inline virtual void writeInt16Type()  {
        signature_.append("n");
    }
    inline virtual void writeInt32Type()  {
        signature_.append("i");
    }
    inline virtual void writeInt64Type()  {
        signature_.append("x");
    }

    inline virtual void writeUInt8Type()  {
        signature_.append("y");
    }
    inline virtual void writeUInt16Type()  {
        signature_.append("q");
    }
    inline virtual void writeUInt32Type()  {
        signature_.append("u");
    }
    inline virtual void writeUInt64Type()  {
        signature_.append("t");
    }


    inline virtual void writeInt8EnumType()  {
        signature_.append("y");
    }
    inline virtual void writeInt16EnumType()  {
        signature_.append("n");
    }
    inline virtual void writeInt32EnumType()  {
        signature_.append("i");
    }
    inline virtual void writeInt64EnumType()  {
        signature_.append("x");
    }

    inline virtual void writeUInt8EnumType()  {
        signature_.append("y");
    }
    inline virtual void writeUInt16EnumType()  {
        signature_.append("n");
    }
    inline virtual void writeUInt32EnumType()  {
        signature_.append("u");
    }
    inline virtual void writeUInt64EnumType()  {
        signature_.append("t");
    }


    inline virtual void writeFloatType()  {
        signature_.append("d");
    }
    inline virtual void writeDoubleType()  {
        signature_.append("d");
    }

    inline virtual void writeStringType()  {
        signature_.append("s");
    }
    inline virtual void writeByteBufferType()  {
        signature_.append("ay");
    }
    inline virtual void writeVersionType()  {
        signature_.append("(uu)");
    }

    inline virtual void beginWriteStructType()  {
        signature_.append("(");
    }
    inline virtual void endWriteStructType() {
        signature_.append(")");
    }

    inline virtual void beginWriteMapType()  {
        signature_.append("a{");
    }
    inline virtual void endWriteMapType() {
        signature_.append("}");
    }

    inline virtual void writeVectorType()  {
        signature_.append("a");
    }

    inline virtual void writeVariantType()  {
        signature_.append("v");
    }

    inline virtual std::string retrieveSignature() {
        return std::move(signature_);
    }


  private:
    std::string signature_;
};



//##############################################################################################################



template<typename _Type, bool _Check>
struct TypeWriter {
inline static void writeType(TypeStream& typeStream) {
//    if() {
        _Type::writeToTypeStream(typeStream);
//    } else if(std::is_base_of<CommonAPI::SerializableVariant, _Type>::value) {
//        typeStream.writeVariantType();
//    }
}
};



template<typename _Type>
struct TypeWriter<_Type, typename std::enable_if<std::is_base_of<CommonAPI::SerializableVariant, _Type>::value, _Type>::type> {
inline static void writeType(TypeStream& typeStream) {
    typeStream.writeVariantType();
}
};


template<>
struct TypeWriter<bool> {
inline static void writeType(TypeStream& typeStream) {
    typeStream.writeBoolType();
}
};


template<>
struct TypeWriter<int8_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeInt8Type();
}
};

template<>
struct TypeWriter<int16_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeInt16Type();
}
};

template<>
struct TypeWriter<int32_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeInt32Type();
}
};

template<>
struct TypeWriter<int64_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeInt64Type();
}
};


template<>
struct TypeWriter<uint8_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeUInt8Type();
}
};

template<>
struct TypeWriter<uint16_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeUInt16Type();
}
};

template<>
struct TypeWriter<uint32_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeUInt32Type();
}
};

template<>
struct TypeWriter<uint64_t> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeUInt64Type();
}
};


template<>
struct TypeWriter<float> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeFloatType();
}
};

template<>
struct TypeWriter<double> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeDoubleType();
}
};


template<>
struct TypeWriter<std::string> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeStringType();
}
};

template<>
struct TypeWriter<CommonAPI::ByteBuffer> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeByteBufferType();
}
};

template<>
struct TypeWriter<CommonAPI::Version> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeVersionType();
}
};

template<>
struct TypeWriter<CommonAPI::SerializableVariant> {
inline static void writeType (TypeStream& typeStream) {
    typeStream.writeVariantType();
}
};


template<typename _VectorElementType>
struct TypeWriter<std::vector<_VectorElementType>> {
inline static void writeType(TypeStream& typeStream) {
    typeStream.writeVectorType();
    TypeWriter<_VectorElementType>::writeType(typeStream);
}
};


template<typename _KeyType, typename _ValueType>
struct TypeWriter<std::unordered_map<_KeyType, _ValueType>> {
inline static void writeType(TypeStream& typeStream) {
    typeStream.beginWriteMapType();

    TypeWriter<_KeyType>::writeType(typeStream);
    TypeWriter<_ValueType>::writeType(typeStream);

    typeStream.endWriteMapType();
}
};



//##############################################################################################################



struct TypeSearchVisitor {
public:
    TypeSearchVisitor(TypeStream& typeStream): typeStream_(typeStream) {
    }

    template<typename _Type>
    void operator()(const _Type& currentType) const {
        TypeWriter<_Type>::writeType(typeStream_, currentType);
    }

private:
    TypeStream& typeStream_;
};



//##############################################################################################################



class TypeStreamTest: public ::testing::Test {
  protected:

    void SetUp() {
    }

    void TearDown() {
    }
};



TEST_F(TypeStreamTest, CreatesBoolSignature) {
    DBusTypeStream typeStream;
    TypeWriter<bool>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("b") == 0);
}

TEST_F(TypeStreamTest, CreatesInt8Signature) {
    DBusTypeStream typeStream;
    TypeWriter<int8_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeStreamTest, CreatesInt16Signature) {
    DBusTypeStream typeStream;
    TypeWriter<int16_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("n") == 0);
}
TEST_F(TypeStreamTest, CreatesInt32Signature) {
    DBusTypeStream typeStream;
    TypeWriter<int32_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("i") == 0);
}
TEST_F(TypeStreamTest, CreatesInt64Signature) {
    DBusTypeStream typeStream;
    TypeWriter<int64_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("x") == 0);
}

TEST_F(TypeStreamTest, CreatesUInt8Signature) {
    DBusTypeStream typeStream;
    TypeWriter<uint8_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("y") == 0);
}
TEST_F(TypeStreamTest, CreatesUInt16Signature) {
    DBusTypeStream typeStream;
    TypeWriter<uint16_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("q") == 0);
}
TEST_F(TypeStreamTest, CreatesUInt32Signature) {
    DBusTypeStream typeStream;
    TypeWriter<uint32_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("u") == 0);
}
TEST_F(TypeStreamTest, CreatesUInt64Signature) {
    DBusTypeStream typeStream;
    TypeWriter<uint64_t>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("t") == 0);
}

TEST_F(TypeStreamTest, CreatesFloatSignature) {
    DBusTypeStream typeStream;
    TypeWriter<float>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}
TEST_F(TypeStreamTest, CreatesDoubleSignature) {
    DBusTypeStream typeStream;
    TypeWriter<double>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("d") == 0);
}

TEST_F(TypeStreamTest, CreatesStringSignature) {
    DBusTypeStream typeStream;
    TypeWriter<std::string>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("s") == 0);
}

TEST_F(TypeStreamTest, CreatesByteBufferSignature) {
    DBusTypeStream typeStream;
    TypeWriter<CommonAPI::ByteBuffer>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("ay") == 0);
}
TEST_F(TypeStreamTest, CreatesVersionSignature) {
    DBusTypeStream typeStream;
    TypeWriter<CommonAPI::Version>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("(uu)") == 0);
}

TEST_F(TypeStreamTest, CreatesVectorOfStringsSignature) {
    DBusTypeStream typeStream;
    TypeWriter<std::vector<std::string> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("as") == 0);
}

TEST_F(TypeStreamTest, CreatesVectorOfVersionsSignature) {
    DBusTypeStream typeStream;
    TypeWriter<std::vector<CommonAPI::Version> >::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("a(uu)") == 0);
}

TEST_F(TypeStreamTest, CreatesMapOfUInt16ToStringSignature) {
    DBusTypeStream typeStream;
    TypeWriter<std::unordered_map<uint16_t, std::string>>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("a{qs}") == 0);
}

TEST_F(TypeStreamTest, CreatesBasicVariantSignature) {
    DBusTypeStream typeStream;
    TypeWriter<CommonAPI::SerializableVariant>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("v") == 0);
}

TEST_F(TypeStreamTest, CreatesDerivedVariantSignature) {
    DBusTypeStream typeStream;
    TypeWriter<CommonAPI::Variant<int, double, std::string>>::writeType(typeStream);
    std::string signature = typeStream.retrieveSignature();
    ASSERT_TRUE(signature.compare("v") == 0);
}





int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
