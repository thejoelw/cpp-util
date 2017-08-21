#ifndef JWUTIL_SIGNAL_H
#define JWUTIL_SIGNAL_H

#include <assert.h>
#include <type_traits>
#include <vector>
#include <algorithm>

#include "methodcallback.h"

namespace jw_util
{

template <typename... ArgTypes>
class Signal {
public:
    class Listener {
    public:
        Listener(Signal &signal, const jw_util::MethodCallback<ArgTypes...> &callback_arg)
            : signal(signal)
            , callback(callback_arg)
        {
            enable();
        }

        ~Listener() {
            disable();
        }

        void enable() {
            signal.add(callback);
        }

        void disable() {
            signal.remove(callback);
        }

    private:
        Signal &signal;
        jw_util::MethodCallback<ArgTypes...> callback;
    };

    ~Signal() {
        assert(callbacks.empty());
    }

    void operator() (ArgTypes... args) {
        typename std::vector<jw_util::MethodCallback<ArgTypes...>>::const_iterator i
            = callbacks.cbegin();
        while (i != callbacks.cend()) {
            i->call(std::forward<ArgTypes>(args)...);
            i++;
        }
    }

private:
    void add(const jw_util::MethodCallback<ArgTypes...> &callback) {
        callbacks.push_back(callback);
    }

    void remove(const jw_util::MethodCallback<ArgTypes...> &callback) {
        typename std::vector<jw_util::MethodCallback<ArgTypes...>>::reverse_iterator found
            = std::find(callbacks.rbegin(), callbacks.rend(), callback);
        assert(found != callbacks.rend());
        *found = callbacks.back();
        callbacks.pop_back();
    }

    std::vector<jw_util::MethodCallback<ArgTypes...>> callbacks;
};

}

#endif // JWUTIL_SIGNAL_H
