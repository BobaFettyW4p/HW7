#ifndef LOCK_FREE_STACK_H
#  define LOCK_FREE_STACK_H
#include<atomic>
#include<memory>
using std::atomic;

namespace mpcs51044 {
// Linked list of integers
struct StackItem {
  StackItem(int val) : next(0), value(val) {}
  StackItem *next; // Next item, 0 if this is last
  int value;
};

struct StackHead {
  StackItem *link;      // First item, 0 if list empty
  unsigned count;      // How many times the list has changed (see lecture notes)
};

struct Stack {
  Stack();
  int pop();
  void push(int);
private:
  atomic<StackHead> head;
};

Stack::Stack()
{
  StackHead init;
  init.link = nullptr;
  init.count = 0;
  head.store(init);
}

// Pop value off list
int
Stack::pop()
{
    // What the head will be if nothing messed with it
    StackHead expected = head.load();
    StackHead newHead;
    bool succeeded = false;
    while(!succeeded) {
        if(expected.link == 0) {
            return 0; // List is empty
        }
        // What the head will be after the pop:
        newHead.link = expected.link->next;
        newHead.count = expected.count + 1;
        // Even if the compare_exchange fails, it updates expected.
        succeeded = head.compare_exchange_weak(expected, newHead);
    }
    int value = expected.link->value;
    delete expected.link;
    return value;
}

/**
 * This is my implementation of the push function for this lock free stack
 * Following the form of the provided pop function, we begin by loading the current head and initializing a new StackHead object to serve as the new head
 * We then utilize the same design paradigm from the pop function where we attempt to update the head and utilize compare_exchange_weak() to determine whether the head was successfully updated
 * One way my push function differs from the pop function is I have utilized modern memory management
 * As opposed to calling operator new and delete as the pop() function does when the operation is not successful to avoid memory, I am utilizing unique_ptrs to manage the new item
 * oeprator new and delete are considered thread-safe in C++, but we can lead to undefined behavior were they to fail, so it's better to utilize RAII to manage memory
 */
void
Stack::push(int val)
{
    StackHead expected = head.load();
    StackHead newHead;
    bool succeeded = false;
    std::unique_ptr<StackItem> newItem;
    while(!succeeded) {
        newItem = std::make_unique<StackItem>(val);
        newHead.link = newItem.get();
        newHead.link->next = expected.link;
        newHead.count = expected.count + 1;
        succeeded = head.compare_exchange_weak(expected, newHead);   
    }
    // finally, we release the unique_ptr to the new item
    // this does carry risks in terms of memory management. Were an exception to happen after release() is called but before the new value is inserted into the stack, a memory leak will occur
    // since we do not call release() until we have confirmed that the operation is successful, this risk is minimized
    // a better solution would be to rewrite the entire stack to utilize shared_ptrs, but that's outside the scope of this assignment
    newItem.release();
}
}
#endif