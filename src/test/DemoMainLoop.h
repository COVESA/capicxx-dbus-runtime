/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DEMO_MAIN_LOOP_H_
#define DEMO_MAIN_LOOP_H_


#include <CommonAPI/MainLoopContext.h>

#include <vector>
#include <set>
#include <map>
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <cassert>


namespace CommonAPI {

class MainLoop {
 public:
    MainLoop() = delete;
    MainLoop(const MainLoop&) = delete;
    MainLoop& operator=(const MainLoop&) = delete;
    MainLoop(MainLoop&&) = delete;
    MainLoop& operator=(MainLoop&&) = delete;

    explicit MainLoop(std::shared_ptr<MainLoopContext> context) :
            context_(context), currentMinimalTimeoutInterval_(TIMEOUT_INFINITE), running_(false), breakLoop_(false) {
        wakeFd_.fd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
        wakeFd_.events = POLLIN | POLLOUT | POLLERR;

        assert(wakeFd_.fd != -1);
        registerFileDescriptor(wakeFd_);

        dispatchSourceListenerSubscription_ = context_->subscribeForDispatchSources(
                std::bind(&CommonAPI::MainLoop::registerDispatchSource, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::MainLoop::deregisterDispatchSource, this, std::placeholders::_1));
        watchListenerSubscription_ = context_->subscribeForWatches(
                std::bind(&CommonAPI::MainLoop::registerWatch, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::MainLoop::deregisterWatch, this, std::placeholders::_1));
        timeoutSourceListenerSubscription_ = context_->subscribeForTimeouts(
                std::bind(&CommonAPI::MainLoop::registerTimeout, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::MainLoop::deregisterTimeout, this, std::placeholders::_1));
        wakeupListenerSubscription_ = context_->subscribeForWakeupEvents(
                std::bind(&CommonAPI::MainLoop::wakeup, this));
    }

    ~MainLoop() {
        deregisterFileDescriptor(wakeFd_);

        context_->unsubscribeForDispatchSources(dispatchSourceListenerSubscription_);
        context_->unsubscribeForWatches(watchListenerSubscription_);
        context_->unsubscribeForTimeouts(timeoutSourceListenerSubscription_);
        context_->unsubscribeForWakeupEvents(wakeupListenerSubscription_);

        close(wakeFd_.fd);
    }

    /**
     * The given timeout will be overridden if a timeout-event is present that defines an earlier ready time.
     */
    void run(const int64_t& timeoutInterval = TIMEOUT_INFINITE) {
        running_ = true;
        while(running_) {
            doSingleIteration(timeoutInterval);
        }
    }

    void stop() {
        running_ = false;
        wakeup();
    }

    /**
     * \brief Executes a single cycle of the mainloop.
     *
     * Subsequently calls prepare(), poll(), check() and, if necessary, dispatch().
     * The given timeout (milliseconds) represents the maximum time
     * this iteration will remain in the poll state. All other steps
     * are handled in a non-blocking way. Note however that a source
     * might claim to have infinite amounts of data to dispatch.
     * This demo-implementation of a Mainloop will dispatch a source
     * until it no longer claims to have data to dispatch.
     * Dispatch will not be called if no sources, watches and timeouts
     * claim to be ready during the check()-phase.
     *
     * @param timeout The maximum poll-timeout for this iteration.
     */
    void doSingleIteration(const int64_t& timeout = TIMEOUT_INFINITE) {
        prepare(timeout);
        poll();
        if(check()) {
            dispatch();
        }
    }

    /*
     * The given timeout is a maximum timeout in ms, measured from the current time in the future
     * (a value of 0 means "no timeout"). It will be overridden if a timeout-event is present
     * that defines an earlier ready time.
     */
    void prepare(const int64_t& timeout = TIMEOUT_INFINITE) {
        currentMinimalTimeoutInterval_ = timeout;

        for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                        dispatchSourceIterator != registeredDispatchSources_.end();
                        dispatchSourceIterator++) {

            int64_t dispatchTimeout = TIMEOUT_INFINITE;
            if(dispatchSourceIterator->second->prepare(dispatchTimeout)) {
                sourcesToDispatch_.insert(*dispatchSourceIterator);
            } else if (dispatchTimeout < currentMinimalTimeoutInterval_) {
                currentMinimalTimeoutInterval_ = dispatchTimeout;
            }
        }

        int64_t currentContextTime = getCurrentTimeInMs();

        for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                        timeoutPriorityRange != registeredTimeouts_.end();
                        timeoutPriorityRange++) {

            int64_t intervalToReady = timeoutPriorityRange->second->getReadyTime() - currentContextTime;

            if (intervalToReady <= 0) {
                timeoutsToDispatch_.insert(*timeoutPriorityRange);
                currentMinimalTimeoutInterval_ = TIMEOUT_NONE;
            } else if (intervalToReady < currentMinimalTimeoutInterval_) {
                currentMinimalTimeoutInterval_ = intervalToReady;
            }
        }
    }

    void poll() {
        for (auto fileDescriptor = managedFileDescriptors_.begin() + 1; fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
            (*fileDescriptor).revents = 0;
        }

        auto numReadyFileDescriptors = ::poll(&(managedFileDescriptors_[0]), managedFileDescriptors_.size(), currentMinimalTimeoutInterval_);

        // If no FileDescriptors are ready, poll returned because of a timeout that has expired.
        // The only case in which this is not the reason is when the timeout handed in "prepare"
        // expired before any other timeouts.
        if(!numReadyFileDescriptors) {
            int64_t currentContextTime = getCurrentTimeInMs();

            for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                            timeoutPriorityRange != registeredTimeouts_.end();
                            timeoutPriorityRange++) {

                int64_t intervalToReady = timeoutPriorityRange->second->getReadyTime() - currentContextTime;

                if (intervalToReady <= 0) {
                    timeoutsToDispatch_.insert(*timeoutPriorityRange);
                }
            }
        }

        if (wakeFd_.revents) {
            acknowledgeWakeup();
        }
    }

    bool check() {
        //The first file descriptor always is the loop's wakeup-descriptor. All others need to be linked to a watch.
        for (auto fileDescriptor = managedFileDescriptors_.begin() + 1; fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
            for(auto registeredWatchIterator = registeredWatches_.begin();
                        registeredWatchIterator != registeredWatches_.end();
                        registeredWatchIterator++) {
                const auto& correspondingWatchPriority = registeredWatchIterator->first;
                const auto& correspondingWatchPair = registeredWatchIterator->second;

                if (std::get<0>(correspondingWatchPair) == fileDescriptor->fd && fileDescriptor->revents) {
                    watchesToDispatch_.insert( { correspondingWatchPriority, {std::get<1>(correspondingWatchPair), fileDescriptor->revents} } );
                }
            }
        }

        for(auto dispatchSourceIterator = registeredDispatchSources_.begin(); dispatchSourceIterator != registeredDispatchSources_.end(); ++dispatchSourceIterator) {
            if((std::get<1>(*dispatchSourceIterator))->check()) {
                sourcesToDispatch_.insert( {std::get<0>(*dispatchSourceIterator), std::get<1>(*dispatchSourceIterator)});
            }
        }

        return !timeoutsToDispatch_.empty() || !watchesToDispatch_.empty() || !sourcesToDispatch_.empty();
    }

    void dispatch() {
        for (auto timeoutIterator = timeoutsToDispatch_.begin();
                timeoutIterator != timeoutsToDispatch_.end();
                timeoutIterator++) {
            std::get<1>(*timeoutIterator)->dispatch();
        }

        for (auto watchIterator = watchesToDispatch_.begin();
                watchIterator != watchesToDispatch_.end();
                watchIterator++) {
            Watch* watch = std::get<0>(watchIterator->second);
            const unsigned int flags = std::get<1>(watchIterator->second);
            const auto& dependentSources = watch->getDependentDispatchSources();
            watch->dispatch(flags);

            for (auto dependentSourceIterator = dependentSources.begin();
                    dependentSourceIterator != dependentSources.end();
                    dependentSourceIterator++) {
                if((*dependentSourceIterator)->check()) {
                    sourcesToDispatch_.insert( {watchIterator->first, *dependentSourceIterator} );
                }
            }
        }

        breakLoop_ = false;
        for (auto dispatchSourceIterator = sourcesToDispatch_.begin();
                        dispatchSourceIterator != sourcesToDispatch_.end() && !breakLoop_;
                        dispatchSourceIterator++) {

            while(std::get<1>(*dispatchSourceIterator)->dispatch());
        }

        timeoutsToDispatch_.clear();
        sourcesToDispatch_.clear();
        watchesToDispatch_.clear();
    }

    void wakeup() {
        uint32_t wake = 1;
        ::write(wakeFd_.fd, &wake, sizeof(uint32_t));
    }

 private:
    void registerFileDescriptor(const pollfd& fileDescriptor) {
        managedFileDescriptors_.push_back(fileDescriptor);
    }

    void deregisterFileDescriptor(const pollfd& fileDescriptor) {
        for (auto it = managedFileDescriptors_.begin(); it != managedFileDescriptors_.end(); it++) {
            if ((*it).fd == fileDescriptor.fd) {
                managedFileDescriptors_.erase(it);
                break;
            }
        }
    }

    void registerDispatchSource(DispatchSource* dispatchSource, const DispatchPriority dispatchPriority) {
        registeredDispatchSources_.insert( {dispatchPriority, dispatchSource} );
    }

    void deregisterDispatchSource(DispatchSource* dispatchSource) {
        for(auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();
                dispatchSourceIterator++) {

            if(dispatchSourceIterator->second == dispatchSource) {
                registeredDispatchSources_.erase(dispatchSourceIterator);
                break;
            }
        }
        breakLoop_ = true;
    }

    void registerWatch(Watch* watch, const DispatchPriority dispatchPriority) {
        registerFileDescriptor(watch->getAssociatedFileDescriptor());
        registeredWatches_.insert( { dispatchPriority, {watch->getAssociatedFileDescriptor().fd, watch} } );
    }

    void deregisterWatch(Watch* watch) {
        deregisterFileDescriptor(watch->getAssociatedFileDescriptor());

        for(auto watchIterator = registeredWatches_.begin();
                watchIterator != registeredWatches_.end();
                watchIterator++) {

            if(watchIterator->second.first == watch->getAssociatedFileDescriptor().fd) {
                registeredWatches_.erase(watchIterator);
                break;
            }
        }
    }

    void registerTimeout(Timeout* timeout, const DispatchPriority dispatchPriority) {
        registeredTimeouts_.insert( {dispatchPriority, timeout} );
    }

    void deregisterTimeout(Timeout* timeout) {
        for(auto timeoutIterator = registeredTimeouts_.begin();
                timeoutIterator != registeredTimeouts_.end();
                timeoutIterator++) {

            if(timeoutIterator->second == timeout) {
                registeredTimeouts_.erase(timeoutIterator);
                break;
            }
        }
    }

    void acknowledgeWakeup() {
        uint32_t buffer;
        while (::read(wakeFd_.fd, &buffer, sizeof(uint32_t)) == sizeof(buffer));
    }

    std::shared_ptr<MainLoopContext> context_;

    std::vector<pollfd> managedFileDescriptors_;

    std::multimap<DispatchPriority, DispatchSource*> registeredDispatchSources_;
    std::multimap<DispatchPriority, std::pair<int, Watch*>> registeredWatches_;
    std::multimap<DispatchPriority, Timeout*> registeredTimeouts_;

    std::set<std::pair<DispatchPriority, DispatchSource*>> sourcesToDispatch_;
    std::set<std::pair<DispatchPriority, std::pair<Watch*, unsigned short>>> watchesToDispatch_;
    std::set<std::pair<DispatchPriority, Timeout*>> timeoutsToDispatch_;

    DispatchSourceListenerSubscription dispatchSourceListenerSubscription_;
    WatchListenerSubscription watchListenerSubscription_;
    TimeoutSourceListenerSubscription timeoutSourceListenerSubscription_;
    WakeupListenerSubscription wakeupListenerSubscription_;

    int64_t currentMinimalTimeoutInterval_;
    bool breakLoop_;
    bool running_;

    pollfd wakeFd_;
};


} // namespace CommonAPI

#endif /* DEMO_MAIN_LOOP_H_ */
