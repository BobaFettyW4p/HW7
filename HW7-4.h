
#include <future>
#include <condition_variable>
#include <mutex>
#include <memory>

namespace mpcs51044 {
    template<typename T>
    class MyFuture {
        // these are the fields of the MyFuture class
        // they are all shared_ptrs to allow for multiple MyFuture objects to share the same resource
        std::shared_ptr<T> value;
        std::shared_ptr<bool> ready;
        std::shared_ptr<std::mutex> mutex;
        std::shared_ptr<std::condition_variable> condition;
        std::shared_ptr<std::exception_ptr> error;
    public:
        // constructor that initializes the future object
        // ready initializes to false because the future object is not ready to be used. The set_value() and set_exception() functions will set it to true
        MyFuture() : 
            value(std::make_shared<T>()),
            ready(std::make_shared<bool>(false)),
            mutex(std::make_shared<std::mutex>()),
            condition(std::make_shared<std::condition_variable>()),
            error(std::make_shared<std::exception_ptr>()) {}

        /**
         * @brief Get the value of the future object
         * @return T
         * public getter for the future object
         * The mutex, ready, value and error fields are all managed by shared pointers, so we need to dereference them to use them
         * If we didn't want to do this, we could rewrite them to all point to structs with public getter and setter methods (condition_variable is the struct that has this for our condition field)
         */
        T get() {
            std::unique_lock<std::mutex> lock(*mutex);
            condition->wait(lock, [this] { return *ready; });
            if (*error) { 
                std::rethrow_exception(*error);
            }
            return *value;
        }

        /**
         * @brief Set the value of the future object
         * @param val
         * public setter for the future object
         */
        void set_value(T val) {
            std::lock_guard<std::mutex> lock(*mutex);
            *value = val;
            *ready = true;
            condition->notify_one();
        }
        /**
         * @brief Set the exception of the future object
         * @param e
         * public setter for the future object
         * this provides for error handling across threads
         */
        void set_exception(std::exception_ptr e) {
            std::lock_guard<std::mutex> lock(*mutex);
            *error = e;
            *ready = true;
            condition->notify_one();
        }
    };

    template<typename T>
    class MyPromise {
        // field of the MyPromise class that is a MyFuture instance
        MyFuture<T> future;
    public:
        // constructor that initializes the MyPromise instance with a MyFuture instance
        MyPromise() : future() {}

        /**
         * @brief Get the future object
         * @return MyFuture<T>
         * public getter for the future object
         */
        MyFuture<T> get_future() {
            return future;
        }

        /**
         * @brief Set the value of the future object
         * @param val
         * public setter for the future object
         */
        void set_value(T val) {
            future.set_value(val);
        }

        /**
         * @brief Set the exception of the future object
         * @param e
         * sets the exception of the future object
         * this provides for error handling across threads
         */
        void set_exception(std::exception_ptr e) {
            future.set_exception(e);
        }
    };
}