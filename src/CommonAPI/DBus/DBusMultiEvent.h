/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_MULTI_EVENT_H_
#define COMMONAPI_DBUS_DBUS_MULTI_EVENT_H_

#include <CommonAPI/Event.h>

#include <string>
#include <unordered_map>


namespace CommonAPI {
namespace DBus {

template <typename... _Arguments>
class DBusMultiEvent {
 public:
	typedef std::function<SubscriptionStatus(const std::string&, const _Arguments&...)> Listener;
	typedef std::unordered_multimap<std::string, Listener> ListenersMap;
	typedef typename ListenersMap::iterator Subscription;

	Subscription subscribeAll(const Listener& listener);
	Subscription subscribe(const std::string& eventName, const Listener& listener);

	void unsubscribe(Subscription listenerSubscription);

	virtual ~DBusMultiEvent() { }

 protected:
	SubscriptionStatus notifyListeners(const std::string& name, const _Arguments&... eventArguments);

	virtual void onFirstListenerAdded(const std::string& name, const Listener& listener) { }
	virtual void onListenerAdded(const std::string& name, const Listener& listener) { }

	virtual void onListenerRemoved(const std::string& name, const Listener& listener) { }
	virtual void onLastListenerRemoved(const std::string& name, const Listener& listener) { }

 private:
	typedef std::pair<typename ListenersMap::iterator, typename ListenersMap::iterator> IteratorRange;
	SubscriptionStatus notifyListenersRange(const std::string& name, IteratorRange listenersRange, const _Arguments&... eventArguments);

	ListenersMap listenersMap_;
};

template <typename... _Arguments>
typename DBusMultiEvent<_Arguments...>::Subscription
DBusMultiEvent<_Arguments...>::subscribeAll(const Listener& listener) {
	return subscribe(std::string(), listener);
}

template <typename... _Arguments>
typename DBusMultiEvent<_Arguments...>::Subscription
DBusMultiEvent<_Arguments...>::subscribe(const std::string& eventName, const Listener& listener) {
	const bool firstListenerAdded = listenersMap_.empty();

	auto listenerSubscription = listenersMap_.insert({eventName, listener});

	if (firstListenerAdded) {
		onFirstListenerAdded(eventName, listener);
	}

	onListenerAdded(eventName, listener);

	return listenerSubscription;
}

template <typename... _Arguments>
void DBusMultiEvent<_Arguments...>::unsubscribe(Subscription listenerSubscription) {
	const std::string name = listenerSubscription->first;
	const Listener listener = listenerSubscription->second;

	listenersMap_.erase(listenerSubscription);

	onListenerRemoved(name, listener);

	const bool lastListenerRemoved = listenersMap_.empty();
	if (lastListenerRemoved)
		onLastListenerRemoved(name, listener);
}

template <typename... _Arguments>
SubscriptionStatus DBusMultiEvent<_Arguments...>::notifyListeners(const std::string& name, const _Arguments&... eventArguments) {
	const SubscriptionStatus subscriptionStatus = notifyListenersRange(name, listenersMap_.equal_range(name), eventArguments...);

	if (subscriptionStatus == SubscriptionStatus::CANCEL)
		return SubscriptionStatus::CANCEL;

	return notifyListenersRange(name, listenersMap_.equal_range(std::string()), eventArguments...);
}

template <typename... _Arguments>
SubscriptionStatus DBusMultiEvent<_Arguments...>::notifyListenersRange(
		const std::string& name,
		IteratorRange listenersRange,
		const _Arguments&... eventArguments) {
	for (auto iterator = listenersRange.first; iterator != listenersRange.second; iterator++) {
		const Listener& listener = iterator->second;
		const SubscriptionStatus listenerSubcriptionStatus = listener(name, eventArguments...);

		if (listenerSubcriptionStatus == SubscriptionStatus::CANCEL) {
			auto listenerIterator = iterator;
			listenersMap_.erase(listenerIterator);
		}
	}

	return listenersMap_.empty() ? SubscriptionStatus::CANCEL : SubscriptionStatus::RETAIN;
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_MULTI_EVENT_H_
