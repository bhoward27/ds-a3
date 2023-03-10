#include <iostream>
#include <mutex>

#include "../common/allocator.h"

template <class T>
struct Node {
    T value;
    Node<T>* next;
};

template <class T>
class TwoLockQueue {
    private:
        CustomAllocator my_allocator_;
        Node<T>* head;
        Node<T>* tail;
        std::mutex head_lock;
        std::mutex tail_lock;

    public:
        TwoLockQueue() : my_allocator_()
        {
            std::cout << "Using TwoLockQueue\n";
        }

        void initQueue(long t_my_allocator_size){
            std::cout << "Using Allocator\n";
            my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
            Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            head = tail = node;
        }

        void enqueue(T value)
        {
            Node<T>* node = static_cast<Node<T>*>(my_allocator_.newNode());
            node->value = value;
            node->next = nullptr;

            tail_lock.lock();
            {
                tail->next = node;
                tail = node;
            }
            tail_lock.unlock();
        }

        bool dequeue(T* p_value)
        {
            Node<T>* old_head;
            Node<T>* new_head;
            head_lock.lock();
            {
                old_head = head;
                new_head = old_head->next;
                if (new_head == nullptr) {
                    head_lock.unlock();
                    return false;
                }
                *p_value = new_head->value;
                head = new_head;
            }
            head_lock.unlock();

            my_allocator_.freeNode(old_head);
            return true;
        }

        void cleanup()
        {
            my_allocator_.cleanup();
        }
};