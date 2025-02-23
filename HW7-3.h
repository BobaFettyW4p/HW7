#include <future>
#include <functional>
#include <memory>

namespace mpcs51044 {
    //forward template declaration because we're using template specialization
    template<typename T>
    class my_packaged_task;

    //specialized template that accepts a function type R(Args...)
    template<typename R, typename... Args>
    class my_packaged_task<R(Args...)> {
    private:
        std::promise<R> promise;
        std::function<R(Args...)> func;
        bool valid = true;

    public:
        //this is our constructor for the packaged task class
        //we leverage an rvalue reference (F&&) to allow the constructor accept both regular functions, as well as functions being moved into the class
        template<typename F>
        explicit my_packaged_task(F&& f) : func(std::forward<F>(f)) {}

        //Since promises can only be moved, adding a copy constructor this class doesn't make sense
        //Thus, we have added a move constructor that moves the promise and function from one packaged task to another
        my_packaged_task(my_packaged_task&& other) noexcept
            : promise(std::move(other.promise))
            , func(std::move(other.func))
            , valid(other.valid) {
            other.valid = false;
        }
        //explicitly delete copy constructor and assignment operator for clarity
        my_packaged_task(const my_packaged_task&) = delete;
        my_packaged_task& operator=(const my_packaged_task&) = delete;

        /**
         * @brief Get the future object
         * returns a future object that will hold the result of the packaged task
         */
        std::future<R> get_future() {
            return promise.get_future();
        }

        /**
         * @brief Call operator
         * @param args: the arguments to pass to the function
         * This function is used to call the function that we stored in the class
         * and would be the function that is generally considered to be the "task"
         * It can either return a value, void, or throw an exception, just as a normal packaged_task would
         */
        void operator()(Args... args) {
            try {
                //if the packaged_task has already been moved, throw an exception
                if (!valid) {
                    throw std::future_error(std::future_errc::broken_promise);
                }
                //if the function is a void function, we can simply call it and set the promise value (it will evaluate to void)
                if constexpr (std::is_void_v<R>) {
                    func(std::forward<Args>(args)...);
                    promise.set_value();
                } else {
                    //if the function is not a void function, we can call it and set the promise value (it will evaluate to the return value of the function)
                    promise.set_value(func(std::forward<Args>(args)...));
                }
            } catch (...) {
                //if an exception is thrown, we can set the exception in the promise
                promise.set_exception(std::current_exception());
            }
        }
    };
}