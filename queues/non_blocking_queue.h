#include <cstdint>

#include "../common/allocator.h"
#include "../common/utils.h"

#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")

#define LSB_48_BIT_MASK 0x0000FFFFFFFFFFFF

/*
 Assumptions:
 - pointers are 8 bytes
 - unsigned short is 2 bytes
 - right-shift >> on unsigned data does a logical shift (zero-extending) as opposed to an arithmetic shift
   (sign-extending)
*/
template <class P>
struct pointer_t {
    P* ptr;

    P* address()
    {
        // Get the address by getting the 48 least significant bits of ptr
        auto uint_ptr = reinterpret_cast<std::uintptr_t>(ptr);
        return reinterpret_cast<P*>(uint_ptr & LSB_48_BIT_MASK);
    }

    unsigned short count()
    {
        // Get the count from the 16 most significant bits of ptr
        auto uint_ptr = reinterpret_cast<std::uintptr_t>(ptr);
        return (uint_ptr >> 48);
    }
};

template <class T>
struct Node
{
    T value;
    pointer_t<Node<T>> next;
};

template <class T>
class NonBlockingQueue {
    private:
        CustomAllocator my_allocator_;
        pointer_t<Node<T>> head;
        pointer_t<Node<T>> tail;

        pointer_t<Node<T>> combine_and_increment(pointer_t<Node<T>> a, pointer_t<Node<T>> b)
        {
            auto x = reinterpret_cast<Node<T>*>(
                reinterpret_cast<std::uintptr_t>(a.address()) +
                (static_cast<std::uintptr_t>(b.count() + 1) << 48)
            );
            pointer_t<Node<T>> y;
            y.ptr = x;
            return y;
        }

    public:
        NonBlockingQueue() : my_allocator_()
        {
            std::cout << "Using NonBlockingQueue\n";
        }

        void initQueue(long t_my_allocator_size){
            std::cout << "Using Allocator\n";
            my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
            Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            node->next.ptr = nullptr;
            head.ptr = tail.ptr = node;
        }

        void enqueue(T value)
        {
            // Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            Node<T>* a = static_cast<Node<T>*>(my_allocator_.newNode());
            a->value = value;
            a->next.ptr = nullptr;
            pointer_t<Node<T>> node;
            node.ptr = a;
            pointer_t<Node<T>> old_tail;
            SFENCE;
            while (true) {
                old_tail = tail;
                LFENCE;
                pointer_t<Node<T>> next = tail.address()->next;
                LFENCE;
                if (tail.ptr == old_tail.ptr) {
                    if (next.address() == nullptr) {
                        /*
                         What the below line is trying to do...
                         node is a pointer to the new node we want to enqueue into the list.
                         We're trying to atomically set the tail->next to point to node. And we want tail->next's count
                         to be incremented by 1.
                         So, we want tail->next.address() to point to node,
                         and we want tail->next.count() to be incremented by 1.
                        */
                        if (CAS(&old_tail.address()->next, next, combine_and_increment(node, next))) {
                            break;
                        }
                    }
                    else {
                        CAS(&tail, old_tail, combine_and_increment(next, old_tail));
                    }
                }
            }
            SFENCE;
            CAS(&tail, old_tail, combine_and_increment(node, old_tail));
        }

        bool dequeue(T* p_value)
        {
            // Use LFENCE and SFENCE as mentioned in pseudocode
            pointer_t<Node<T>> old_head;
            while (true) {
                old_head = head;
                LFENCE;
                pointer_t<Node<T>> old_tail = tail;
                LFENCE;
                pointer_t<Node<T>> next = head.address()->next;
                LFENCE;
                if (old_head.ptr == head.ptr) {
                    if (old_head.address() == old_tail.address()) {
                        if (next.address() == nullptr) {
                            return false;
                        }
                        CAS(&tail, old_tail, combine_and_increment(next, old_tail));
                    }
                    else {
                        *p_value = next.address()->value;
                        if (CAS(&head, old_head, combine_and_increment(next, old_head))) {
                            break;
                        }
                    }
                }
            }
            my_allocator_.freeNode(old_head.address());
            return true;
        }

        void cleanup()
        {
            my_allocator_.cleanup();
        }

};

