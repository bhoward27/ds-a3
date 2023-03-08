#include "../common/allocator.h"

template <class T>
class Node
{
};

template <class T>
class LockFreeStack
{
    CustomAllocator my_allocator_;
public:
    LockFreeStack() : my_allocator_()
    {
        std::cout << "Using LockFreeStack\n";
    }

    void initStack(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Perform any necessary initializations
    }

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        return false;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
