#include <future>

namespace mpcs51044 {
    // This is a template declaration for mpcs51044:packaged_task function that specifies a function type Func and a variadic list of arguments Args
    template<typename Func, typename... Args>
    auto my_packaged_task(Func f, Args... args) {
        //initialize a promise and future object
        auto promise = std::promise<decltype(f(args...))>();
        auto future = promise.get_future();

        // moving the promise object to the thread to avoid copying
        // if we didn't do this, passing the promise object to the thread would mean the promise object would not outlive the thread
        auto p = std::move(promise);

        //creating a thread that will execute the function f with the arguments args and the promise p
        std::thread t([f, args..., p = std::move(p)]() mutable {
            try {
                // if calling the function f with the arguments args returns void, we set the value of the promise to void...
                if constexpr (std::is_void_v<decltype(f(args...))>) {
                    f(args...);
                    p.set_value();
                } else {
                // otherwise, we set the value of the promise to the result of the function f with the arguments args
                    p.set_value(f(args...));
                }
            } catch(...) {
                // if an exception is thrown, we set the exception of the promise to the exception
                p.set_exception(std::current_exception());
            }
        });
        t.detach();
        return future;
    }
}