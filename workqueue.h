#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <array>
#include <tuple>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "libBlobPlanet/util/methodcallback.h"

namespace jw_util
{

template <unsigned int num_threads, typename... ArgTypes>
class WorkQueue
{
public:
    WorkQueue(jw_util::MethodCallback<ArgTypes...> worker)
        : worker(worker)
    {
        for (unsigned int i = 0; i < num_threads; i++)
        {
            threads[i] = std::thread(&WorkQueue<num_threads, ArgTypes...>::loop, this);
        }
    }

    ~WorkQueue()
    {
        shut_down();

        for (unsigned int i = 0; i < num_threads; i++)
        {
            threads[i].join();
        }
    }

    void push(ArgTypes... args)
    {
        assert(running);

        if (num_threads)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                (void) lock;
                queue.emplace(std::forward<ArgTypes>(args)...);
            }
            conditional_variable.notify_one();
        }
        else
        {
            worker.call(std::forward<ArgTypes>(args)...);
        }
    }

    void shut_down()
    {
        if (running)
        {
            running = false;

            if (num_threads)
            {
                conditional_variable.notify_all();
            }
        }
    }

private:
    typedef std::tuple<typename std::remove_reference<ArgTypes>::type...> TupleType;

    const jw_util::MethodCallback<ArgTypes...> worker;

    std::array<std::thread, num_threads> threads;

    std::mutex mutex;
    std::condition_variable conditional_variable;

    std::queue<TupleType> queue;

    bool running = true;

    void loop()
    {
        assert(num_threads);

        std::unique_lock<std::mutex> lock(mutex);
        while (true)
        {
            if (queue.empty())
            {
                if (!running) {break;}
                conditional_variable.wait(lock);
            }
            else
            {
                TupleType args = std::move(queue.front());
                queue.pop();

                lock.unlock();
                dispatch(std::move(args));
                lock.lock();
            }
        }
    }

    void dispatch(TupleType &&args)
    {
        call(std::forward<TupleType>(args), std::index_sequence_for<ArgTypes...>{});
    }

    template<std::size_t... Indices>
    void call(TupleType &&args, std::index_sequence<Indices...>)
    {
        worker.call(std::forward<ArgTypes>(std::get<Indices>(std::forward<TupleType>(args)))...);
    }
};

}

#endif // WORKQUEUE_H
