// Copyright (C) 2014-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#include <CommonAPI/DBus/DBusTypeOutputStream.hpp>

#ifndef COMMONAPI_DBUS_FREEDESKTOPVARIANT_HPP_
#define COMMONAPI_DBUS_FREEDESKTOPVARIANT_HPP_

namespace CommonAPI {
namespace DBus {

template<class Visitor, class Variant, typename ... _Types>
struct ApplyTypeCompareVisitor;

template<class Visitor, class Variant>
struct ApplyTypeCompareVisitor<Visitor, Variant> {
    static const uint8_t index = 0;

    static uint8_t visit(Visitor&, const Variant&) {
        // won't be called if the variant contains the requested type
        assert(false);
        return 0;
    }
};

template<class Visitor, class Variant, typename _Type, typename ... _Types>
struct ApplyTypeCompareVisitor<Visitor, Variant, _Type, _Types...> {
    static const uint8_t index
		= ApplyTypeCompareVisitor<Visitor, Variant, _Types...>::index + 1;

    static uint8_t visit(Visitor &_visitor, const Variant &_variant) {
        DBusTypeOutputStream output;
        _Type current;
        output.writeType(current);
#ifdef WIN32
        if (_visitor.operator()<_Type>(output.getSignature())) {
#else
        if (_visitor.template operator()<_Type>(output.getSignature())) {
#endif
            return index;
        } else {
            return ApplyTypeCompareVisitor<
            			Visitor, Variant, _Types...
				   >::visit(_visitor, _variant);
        }
    }
};

template<typename ... _Types>
struct TypeCompareVisitor {
public:
	TypeCompareVisitor(const std::string &_type)
		: type_(_type) {
	}

	template<typename _Type>
	bool operator()(const std::string &_type) const {
		return (_type == type_);
	}

private:
	const std::string type_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_FREEDESKTOPVARIANT_HPP_
