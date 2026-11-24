#ifndef UNDO_STACK_H
#define UNDO_STACK_H

#include "item.h"
#include <string>

using std::string;

// Describes an operation that can be reversed.
struct Action
{
    string kind;        // "ADD", "DELETE" or "CLAIM"
    Item item;
    string desc;
    string claimant;    // only used when kind is "CLAIM"
    string claimTime;   // only used when kind is "CLAIM"
};

// Array-based stack that stores the most recent actions first.
class UndoStack
{
private:
    Action* data;
    int top;
    int capacityValue;

public:
    // Creates an empty stack with the requested capacity.
    explicit UndoStack(int capacity = 50)
    {
        top = 0;
        capacityValue = capacity;
        data = new Action[capacityValue];
    }

    // Releases the allocated array.
    ~UndoStack()
    {
        delete[] data;
    }

    // Copying is disabled to prevent shared ownership of the array.
    UndoStack(const UndoStack&) = delete;
    UndoStack& operator=(const UndoStack&) = delete;

    // Pushes a new action onto the top of the stack.
    // If the stack is full, the oldest action is discarded.
    void push(const Action& action)
    {
        if (top == capacityValue)
        {
            for (int i = 1; i < capacityValue; i++)
            {
                data[i - 1] = data[i];
            }

            top--;
        }

        data[top] = action;
        top++;
    }

    // Removes and returns the most recent action.
    // Returns false when the stack is empty.
    bool pop(Action& result)
    {
        if (top == 0)
        {
            return false;
        }

        top--;
        result = data[top];

        return true;
    }

    int size() const
    {
        return top;
    }

    // Returns an action by its distance from the top.
    const Action& fromTop(int index) const
    {
        return data[top - 1 - index];
    }
};

#endif // UNDO_STACK_H
