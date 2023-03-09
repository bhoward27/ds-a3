#include <mutex>
#include <iostream>

#include "../common/allocator.h"

template <class T>
struct Node
{
    T value;
    Node<T>* next;
};

template <class T>
class OneLockQueue
{
    private:
        Node<T>* q_head;
        Node<T>* q_tail;
        CustomAllocator my_allocator_;
        std::mutex lock;

    public:
        OneLockQueue() : my_allocator_()
        {
            std::cout << "Using OneLockQueue\n";
        }

        void initQueue(long t_my_allocator_size)
        {
            std::cout << "Using Allocator\n";
            my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
            Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            node->next = nullptr;
            q_head = q_tail = node;
        }

        void enqueue(T value)
        {
            Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            node->value = value;
            node->next = nullptr;

            lock.lock();
            {
                q_tail->next = node;
                q_tail = node;
            }
            lock.unlock();
        }

        bool dequeue(T* p_value)
        {
            Node<T>* old_head;
            Node<T>* new_head;

            lock.lock();
            {
                old_head = q_head;
                new_head = q_head->next;
                if (new_head == nullptr) {
                    // Queue is empty.
                    lock.unlock();
                    return false;
                }
                *p_value = new_head->value;
                q_head = new_head;
            }
            lock.unlock();

            my_allocator_.freeNode(old_head);
            return true;
        }

        void cleanup()
        {
            my_allocator_.cleanup();
        }
};