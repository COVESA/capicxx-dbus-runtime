// Copyright (C) 2014-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef COMMONAPI_DBUS_DBUSTYPEOUTPUTSTREAM_H_
#define COMMONAPI_DBUS_DBUSTYPEOUTPUTSTREAM_H_

#include <CommonAPI/TypeOutputStream.hpp>

namespace CommonAPI {
namespace DBus {

class DBusTypeOutputStream: public TypeOutputStream<DBusTypeOutputStream> {
public:
    DBusTypeOutputStream() : signature_("") {}

    TypeOutputStream &writeType(const bool &_type) {
        signature_.append("b");
        return (*this);
    }

    TypeOutputStream &writeType(const int8_t &) {
        signature_.append("y");
        return (*this);
    }

    TypeOutputStream &writeType(const int16_t &) {
        signature_.append("n");
        return (*this);
    }

    TypeOutputStream &writeType(const int32_t &) {
        signature_.append("i");
        return (*this);
    }

    TypeOutputStream &writeType(const  int64_t &) {
        signature_.append("x");
        return (*this);
    }

    TypeOutputStream &writeType(const uint8_t &) {
        signature_.append("y");
        return (*this);
    }

    TypeOutputStream &writeType(const uint16_t &) {
        signature_.append("q");
        return (*this);
    }

    TypeOutputStream &writeType(const uint32_t &) {
        signature_.append("u");
        return (*this);
    }

    TypeOutputStream &writeType(const uint64_t &) {
        signature_.append("t");
        return (*this);
    }

    TypeOutputStream &writeType(const float &) {
        signature_.append("d");
        return (*this);
    }

    TypeOutputStream &writeType(const double &) {
        signature_.append("d");
        return (*this);
    }

    TypeOutputStream &writeType(const std::string &) {
        signature_.append("s");
        return (*this);
    }

    TypeOutputStream &writeType() {
        signature_.append("ay");
        return (*this);
    }

    TypeOutputStream &writeVersionType() {
        signature_.append("(uu)");
        return (*this);
    }

    template<typename... _Types>
    TypeOutputStream &writeType(const Struct<_Types...> &_value) {
    	signature_.append("(");
        const auto itsSize(std::tuple_size<std::tuple<_Types...>>::value);
        StructTypeWriter<itsSize-1, DBusTypeOutputStream, Struct<_Types...>>{}
        	(*this, _value);
        signature_.append(")");
        return (*this);
    }

    template<class _PolymorphicStruct>
    TypeOutputStream &writeType(const std::shared_ptr<_PolymorphicStruct> &_value) {
    	signature_.append("(");
    	_value->writeType(*this);
        signature_.append(")");
        return (*this);
    }

    template<typename... _Types>
    TypeOutputStream &writeType(const Variant<_Types...> &_value) {
    	signature_.append("(yv)");
    	return (*this);
    }

    template<typename _Deployment, typename... _Types>
    TypeOutputStream &writeType(const Variant<_Types...> &_value, const _Deployment *_depl) {
    	if (_depl != nullptr && _depl->isFreeDesktop_) {
    		signature_.append("v");
    	} else {
    		signature_.append("(yv)");
    	}
		TypeOutputStreamWriteVisitor<DBusTypeOutputStream> typeVisitor(*this);
		ApplyVoidVisitor<TypeOutputStreamWriteVisitor<DBusTypeOutputStream>,
				Variant<_Types...>, _Types...>::visit(typeVisitor, _value);
        return (*this);
    }

	template<typename _ElementType>
	TypeOutputStream &writeType(const std::vector<_ElementType> &_value) {
		signature_.append("a");
		return (*this);
	}

	template<typename _KeyType, typename _ValueType, typename _HasherType>
	TypeOutputStream &writeType(const std::unordered_map<_KeyType, _ValueType, _HasherType> &_value) {
		signature_.append("a{");
		_KeyType dummyKey;
		writeType(dummyKey);
		_ValueType dummyValue;
		writeType(dummyValue);
		signature_.append("}");
		return (*this);
	}

    inline std::string getSignature() {
        return std::move(signature_);
    }

private:
    std::string signature_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSTYPEOUTPUTSTREAM_HPP_
