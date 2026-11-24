#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "item.h"

class LinkedList
{
private:
    // Each node stores one item and a pointer to the next node.
    struct Node
    {
        Item data;
        Node* next;

        Node(const Item& value)
        {
            data = value;
            next = nullptr;
        }
    };

    Node* head;
    Node* cursor;   // remembers where first() and next() are
    int count;

public:
    // Starts with an empty linked list.
    LinkedList()
    {
        head = nullptr;
        cursor = nullptr;
        count = 0;
    }

    // Releases every dynamically allocated node.
    ~LinkedList()
    {
        clear();
    }

    // Copying is disabled because this class owns dynamically allocated nodes.
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    // Inserts a new item at the beginning of the list.
    // Returns false if an item with the same ID already exists.
    bool insert(const Item& item)
    {
        if (find(item.id) != nullptr)
        {
            return false;
        }

        Node* newNode = new Node(item);
        newNode->next = head;
        head = newNode;

        count++;
        return true;
    }

    // Searches for an item using its ID.
    // Returns a pointer to the item when found, otherwise nullptr.
    Item* find(int id)
    {
        Node* current = head;

        while (current != nullptr)
        {
            if (current->data.id == id)
            {
                return &current->data;
            }

            current = current->next;
        }

        return nullptr;
    }

    // Removes the node that contains the requested item ID.
    bool remove(int id)
    {
        Node* current = head;
        Node* previous = nullptr;

        while (current != nullptr)
        {
            if (current->data.id == id)
            {
                if (previous != nullptr)
                {
                    previous->next = current->next;
                }
                else
                {
                    head = current->next;
                }

                delete current;
                count--;
                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    // Returns the number of items currently stored in the list.
    int size() const
    {
        return count;
    }

    // Adds an item at the end of the list (used when loading items from the file).
    // Returns false if an item with the same ID already exists.
    bool insertAtEnd(const Item& item)
    {
        if (find(item.id) != nullptr)
        {
            return false;
        }

        Node* newNode = new Node(item);

        if (head == nullptr)
        {
            head = newNode;
        }
        else
        {
            Node* current = head;
            while (current->next != nullptr)
            {
                current = current->next;
            }
            current->next = newNode;
        }

        count++;
        return true;
    }

    // Walking through the list:
    // first() gives the first item and next() gives the following one.
    // Both return nullptr when there are no more items.
    Item* first()
    {
        cursor = head;
        if (cursor == nullptr)
        {
            return nullptr;
        }
        return &cursor->data;
    }

    Item* next()
    {
        if (cursor == nullptr)
        {
            return nullptr;
        }

        cursor = cursor->next;
        if (cursor == nullptr)
        {
            return nullptr;
        }
        return &cursor->data;
    }

    // Deletes all nodes and resets the list to its empty state.
    void clear()
    {
        while (head != nullptr)
        {
            Node* nextNode = head->next;
            delete head;
            head = nextNode;
        }

        cursor = nullptr;
        count = 0;
    }
};

#endif // LINKED_LIST_H
