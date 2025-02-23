#include <future>
#include <condition_variable>
#include <mutex>
#include <memory>

namespace mpcs51044 {    
    // We forward declare MyPromise template so that MyFuture can use it
    template<typename T> class MyPromise;

    // Our MyFuture class only contains a getter method, compared to the MyPromise class, which also has a setter method
    // This is so the structure of our implementation mimics the functionality of std::future and std::promise
    // Promise is the "producer" side, which sets values and exceptions
    // Future is the "consumer" side, which retreives those values and exceptions
    // Just as data only flows one way in the standard library, from promises to their associated futures, our implementation will follow the same structure
    template<typename T>
    class MyFuture {
        // The SharedState construct contains the fields that our MyFuture and MyPromise will use
        struct SharedState {
            T value;
            bool ready = false;
            std::mutex mutex;
            std::condition_variable condition;
            std::exception_ptr error;
        };
        // encapsulating our SharedState class in a shared_ptr allows for thread safety
        // since these fields should all have the same lifetime, storing them in a struct owned by a single pointer is more efficient than storing them in separate pointers that exist outside of the struct in the MyFuture class
        std::shared_ptr<SharedState> state;

        // We want to ensure that MyPromises and MyFutures are used together as they do in the std:: implementation
        // a friend declaration allows MyPromise to access the private members of MyFuture, including the SharedState struct
        // This also enforces that MyFuture objects are created only through MyPromise objects, whcih mimics the behavior of std::future and std::promise
        friend class MyPromise<T>;
        
        //This is the constructor for a MyFuture object
        //We are doing this explicitly to prevent implicit conversions creating valid MyFuture objects, as opposed to being created by a MyPromise object (which is the only way we want to create MyFuture objects)
        explicit MyFuture(std::shared_ptr<SharedState> state) : state(state) {}

    public:
        /**
         * @brief Public getter method for the MyFuture object
         * @return The value of the future (as contained in the value field of the SharedState object)
         * Since the SharedState object is encapsulated in a shared_ptr, we need to use -> notation to access its fields
         */
        T get() {
            std::unique_lock<std::mutex> lock(state->mutex);
            state->condition.wait(lock, [this] { return state->ready; });
            if (state->error) {
                std::rethrow_exception(state->error);
            }
            return state->value;
        }
    };

    template<typename T>
    class MyPromise {
        // We have specify typename to initialize an object of type MyFuture<T>::SharedState in this instance
        // It is only accessible to MyPromise objects due to the friend declaration in the MyFuture class
        // We have chose to have the object live in MyFuture because it conceptually belongs to the MyFuture object
        // Just as creating a MyPromise without a MyFuture doesn't make sense, having a SharedState without a MyFuture doesn't make sense either
        std::shared_ptr<typename MyFuture<T>::SharedState> state;
        // this boolean is used to ensure that the future is not retrieved more than once
        bool future_retrieved = false;

    public:
        // This is the constructor for a MyPromise object
        // It initializes the state pointer to a new SharedState object
        MyPromise() : state(std::make_shared<typename MyFuture<T>::SharedState>()) {}

        // the following 2 lines allow for MyPromise objects to be moved, but not copied
        // which is important for thread safety
        MyPromise(MyPromise&&) = default;
        MyPromise& operator=(MyPromise&&) = default;

        /**
         * @brief Public getter method for the MyFuture object
         * @return A MyFuture object associated with the current MyPromise object
         * This method returns the MyFuture object that is associated with the current MyPromise object
         * It also ensures that the future is not retrieved more than once, mimicking the behavior of std::future
         * This is an important feature for thread safety, if multiple threads all could get the same future object, they would race to consume the value
         */
        MyFuture<T> get_future() {
            if (future_retrieved) {
                throw std::future_error(std::future_errc::future_already_retrieved);
            }
            future_retrieved = true;
            return MyFuture<T>(state);
        }

        /**
         * @brief Public setter method for the MyFuture object
         * @param val The value to set the future to
         * This method sets the value of the future to the value passed in
         * It lives within the MyPromise class to mimic the behavior of std::promise
         */
        void set_value(T val) {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->ready) {
                throw std::future_error(std::future_errc::promise_already_satisfied);
            }
            state->value = val;
            state->ready = true;
            state->condition.notify_one();
        }

        /**
         * @brief Public setter method for the MyFuture object
         * @param e The exception to set the future to
         * This method sets the exception of the future to the exception passed in
         * It lives within the MyPromise class to mimic the behavior of std::promise
         */
        void set_exception(std::exception_ptr e) {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->ready) {
                throw std::future_error(std::future_errc::promise_already_satisfied);
            }
            state->error = e;
            state->ready = true;
            state->condition.notify_one();
        }
    };
}