#include <future>

namespace mpcs51044 {
    // This is a template declaration for mpcs51044:packaged_task function that specifies a function type Func and a variadic list of arguments Args
    template<typename Func, typename... Args>
    auto my_packaged_task(Func f, Args... args) {
        auto promise = std::promise<decltype(f(args...))>();
        auto future = promise.get_future();
        
        auto p = std::move(promise);
        std::thread t([f, args..., p = std::move(p)]() mutable {
            try {
                if constexpr (std::is_void_v<decltype(f(args...))>) {
                    f(args...);
                    p.set_value();
                } else {
                    p.set_value(f(args...));
                }
            } catch(...) {
                p.set_exception(std::current_exception());
            }
        });
        t.detach();
        return future;
    }
}