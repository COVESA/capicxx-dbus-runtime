/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/SerializableVariant.h>



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

    virtual void writeSerializableStructType(const CommonAPI::SerializableStruct& serializableStruct) = 0;
    virtual void writeVariantType(const CommonAPI::SerializableVariant& serializableVariant) = 0;
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

    inline virtual void beginWriteMapType()  {
        signature_.append("{");
    }
    inline virtual void endWriteMapType() {
        signature_.append("}");
    }

    inline virtual void writeSerializableStructType(const CommonAPI::SerializableStruct& serializableStruct)  {
        //TODO
    }
    inline virtual void writeVariantType(const CommonAPI::SerializableVariant& serializableVariant)  {
        //TODO
    }

    inline virtual void writeVectorType()  {
        signature_.append("a");
    }


  private:
    std::string signature_;
};



//##############################################################################################################




template <typename _VectorElementType>
class TypeStreamGenericVectorHelper {
 public:
    static void writeVectorType(TypeStream& typeStream, const std::vector<_VectorElementType>& vectorValue) {
        typeStream.writeVectorType();
        doWriteVectorType(typeStream, vectorValue);
    }

 private:
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<bool>& vectorValue) {
        typeStream.writeBoolType();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<int8_t>& vectorValue) {
        typeStream.writeInt8Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<int16_t>& vectorValue) {
        typeStream.writeInt16Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<int32_t>& vectorValue) {
        typeStream.writeInt32Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<int64_t>& vectorValue) {
        typeStream.writeInt64Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<uint8_t>& vectorValue) {
        typeStream.writeUInt8Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<uint16_t>& vectorValue) {
        typeStream.writeUInt16Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<uint32_t>& vectorValue) {
        typeStream.writeUInt32Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<uint64_t>& vectorValue) {
        typeStream.writeUInt64Type();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<float>& vectorValue) {
        typeStream.writeFloatType();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<double>& vectorValue) {
        typeStream.writeDoubleType();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<std::string>& vectorValue) {
        typeStream.writeStringType();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<CommonAPI::ByteBuffer>& vectorValue) {
        typeStream.writeByteBufferType();
    }
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<CommonAPI::Version>& vectorValue) {
        typeStream.writeVersionType();
    }

    template<typename _InnerVectorElementType>
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<std::vector<_InnerVectorElementType>>& vectorValue) {
        std::vector<_InnerVectorElementType> dummy;
        TypeStreamVectorHelper<_InnerVectorElementType>::writeVectorType(typeStream, dummy);
    }

    template<typename _InnerKeyType, typename _InnerValueType>
    static inline void doWriteVectorType(TypeStream& typeStream, const std::vector<std::unordered_map<_InnerKeyType, _InnerValueType>>& vectorValue) {
        typeStream.writeVectorType();
        typeStream.beginWriteMapType();
        //TODO
        typeStream.endWriteMapType();
    }
};


template <typename _VectorElementType, bool _IsSerializableStruct = false>
struct TypeStreamSerializableStructVectorHelper: public TypeStreamGenericVectorHelper<_VectorElementType> {
};

template <typename _VectorElementType>
struct TypeStreamSerializableStructVectorHelper<_VectorElementType, true> {
    static void writeVectorType(TypeStream& typeStream, const std::vector<_VectorElementType>& vectorValue) {
        typeStream.writeVectorType();
        //TODO: Write actual struct signature
    }
};


template <typename _VectorElementType>
struct TypeStreamVectorHelper: TypeStreamSerializableStructVectorHelper<_VectorElementType,
                                                                        std::is_base_of<CommonAPI::SerializableStruct, _VectorElementType>::value> {
};



//##############################################################################################################




template<typename _Type>
void writeType(TypeStream& typeStream, _Type& currentType);


template<>
inline void writeType<bool>(TypeStream& typeStream, bool& currentType) {
    return typeStream.writeBoolType();
}


template<>
inline void writeType<int8_t> (TypeStream& typeStream, int8_t& currentType) {
    typeStream.writeInt8Type();
}

template<>
inline void writeType<int16_t> (TypeStream& typeStream, int16_t& currentType) {
    typeStream.writeInt16Type();
}

template<>
inline void writeType<int32_t> (TypeStream& typeStream, int32_t& currentType) {
    typeStream.writeInt32Type();
}

template<>
inline void writeType<int64_t> (TypeStream& typeStream, int64_t& currentType) {
    typeStream.writeInt64Type();
}


template<>
inline void writeType<uint8_t> (TypeStream& typeStream, uint8_t& currentType) {
    typeStream.writeUInt8Type();
}

template<>
inline void writeType<uint16_t> (TypeStream& typeStream, uint16_t& currentType) {
    typeStream.writeUInt16Type();
}

template<>
inline void writeType<uint32_t> (TypeStream& typeStream, uint32_t& currentType) {
    typeStream.writeUInt32Type();
}

template<>
inline void writeType<uint64_t> (TypeStream& typeStream, uint64_t& currentType) {
    typeStream.writeUInt64Type();
}


template<>
inline void writeType<float> (TypeStream& typeStream, float& currentType) {
    typeStream.writeFloatType();
}

template<>
inline void writeType<double> (TypeStream& typeStream, double& currentType) {
    typeStream.writeDoubleType();
}


template<>
inline void writeType<std::string> (TypeStream& typeStream, std::string& currentType) {
    typeStream.writeStringType();
}

template<>
inline void writeType<CommonAPI::ByteBuffer> (TypeStream& typeStream, CommonAPI::ByteBuffer& currentType) {
    typeStream.writeByteBufferType();
}

template<>
inline void writeType<CommonAPI::Version> (TypeStream& typeStream, CommonAPI::Version& currentType) {
    typeStream.writeVersionType();
}

template<>
inline void writeType<CommonAPI::SerializableStruct> (TypeStream& typeStream, CommonAPI::SerializableStruct& currentType) {
    typeStream.writeSerializableStructType(currentType);
}


template<typename... _VariantTypes>
inline void writeType(TypeStream& typeStream, const CommonAPI::SerializableVariant& variant) {
    //TODO
}


template<typename _KeyType, typename _ValueType>
inline void writeType(TypeStream& typeStream, const std::unordered_map<_KeyType, _ValueType>& mapValue) {
    typeStream.beginWriteMapType();
    //TODO
    typeStream.endWriteMapType();
}


template<typename _VectorElementType>
inline void writeType(TypeStream& typeStream, const std::vector<_VectorElementType>& vectorValue) {
    TypeStreamVectorHelper<_VectorElementType>::writeVectorType(typeStream, vectorValue);
}













template<typename _Type>
struct TypeSearchVisitor {
public:
    TypeSearchVisitor(typename TypeStream& typeStream): typeStream_(typeStream) {
    }

    template<typename _Type>
    void operator()(const _Type& currentType) const {
        writeType<_Type>(typeStream_, currentType);
    }

private:
    TypeStream& typeStream_;
};



class TypeStreamTest: public ::testing::Test {
  protected:

    void SetUp() {
    }

    void TearDown() {
    }
};


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
