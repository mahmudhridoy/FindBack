#ifndef CLAIM_QUEUE_H
#define CLAIM_QUEUE_H

#include <string>

using std::string;

// Contains the details of a user's claim request.
struct Claim
{
    int itemId = 0;
    string claimant;
    string time;
};

// Circular queue that processes claim requests in FIFO order.
class ClaimQueue
{
private:
    Claim* data;
    int front;
    int count;
    int capacityValue;

public:
    // Creates a queue with a fixed capacity.
    explicit ClaimQueue(int capacity = 20)
    {
        front = 0;
        count = 0;
        capacityValue = capacity;
        data = new Claim[capacityValue];
    }

    // Releases the dynamically allocated array.
    ~ClaimQueue()
    {
        delete[] data;
    }

    // Copying is disabled to prevent two queues owning the same array.
    ClaimQueue(const ClaimQueue&) = delete;
    ClaimQueue& operator=(const ClaimQueue&) = delete;

    // Adds a claim at the rear of the queue.
    // Returns false when the queue is full.
    bool enqueue(const Claim& claim)
    {
        if (count == capacityValue)
        {
            return false;
        }

        int rear = (front + count) % capacityValue;
        data[rear] = claim;
        count++;

        return true;
    }

    // Removes the claim at the front of the queue.
    // Returns false when the queue is empty.
    bool dequeue(Claim& result)
    {
        if (count == 0)
        {
            return false;
        }

        result = data[front];
        front = (front + 1) % capacityValue;
        count--;

        return true;
    }

    // Puts a claim back at the front of the queue (used when a processed claim is undone).
    // Returns false if the queue is full.
    bool addToFront(const Claim& claim)
    {
        if (count == capacityValue)
        {
            return false;
        }

        front = (front - 1 + capacityValue) % capacityValue;
        data[front] = claim;
        count++;
        return true;
    }

    // Returns the claim at a logical position without removing it.
    const Claim& at(int index) const
    {
        return data[(front + index) % capacityValue];
    }

    int size() const
    {
        return count;
    }

    int capacity() const
    {
        return capacityValue;
    }

    int frontIndex() const
    {
        return front;
    }

    int rearIndex() const
    {
        if (count == 0)
        {
            return -1;
        }

        return (front + count - 1) % capacityValue;
    }
};

#endif // CLAIM_QUEUE_H
