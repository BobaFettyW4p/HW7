#include <future>
#include <type_traits>

namespace mpcs51044 {
    // This is a template declaration for my_async that specifies a function type Func and a variadic list of arguments Args
    template<typename Func, typename ...Args>
    auto my_async(Func f, Args... args) noexcept {
        std::packaged_task task([f, args...]() {
            return f(args...);
        });
        
        auto result = task.get_future();
        std::thread thread(std::move(task));
        // We are detaching the thread to allow it to run independently of the main thread instead of joining it
        // This simulates the functionality of std::async
        // It also means that the main thread will not wait for the thread to finish, which can lead to undefined behavior
        // While this simulates the functionality of std::async, it is a design pattern that would have real ramifications if used in a real program
        thread.detach();
        return result;
    }
}
