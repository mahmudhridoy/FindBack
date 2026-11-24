#ifndef MATCH_LIST_H
#define MATCH_LIST_H

#include "item.h"

// A small sorted singly linked list that keeps only the best matches.
// The match with the highest score is always at the front.
class MatchList
{
private:
    struct Node
    {
        Match data;
        Node* next;

        Node(const Match& value)
        {
            data = value;
            next = nullptr;
        }
    };

    Node* head;
    int count;
    int limit;      // the most matches this list will keep

    // Returns true when match a should come before match b.
    // A higher score comes first. When scores are equal, the lower ID comes first.
    bool isBetter(const Match& a, const Match& b) const
    {
        if (a.score != b.score)
        {
            return a.score > b.score;
        }

        return a.id < b.id;
    }

    // Deletes the last node (the weakest match).
    void removeLast()
    {
        if (head == nullptr)
        {
            return;
        }

        if (head->next == nullptr)
        {
            delete head;
            head = nullptr;
            count = 0;
            return;
        }

        Node* current = head;
        while (current->next->next != nullptr)
        {
            current = current->next;
        }

        delete current->next;
        current->next = nullptr;
        count--;
    }

public:
    // Starts empty. maxSize is how many matches to keep (default 5).
    MatchList(int maxSize = 5)
    {
        head = nullptr;
        count = 0;
        limit = maxSize;
    }

    // Releases every node.
    ~MatchList()
    {
        while (head != nullptr)
        {
            Node* nextNode = head->next;
            delete head;
            head = nextNode;
        }
    }

    // Copying is disabled because this class owns dynamically allocated nodes.
    MatchList(const MatchList&) = delete;
    MatchList& operator=(const MatchList&) = delete;

    // Adds a match in its sorted position (best first).
    // If the list now has more than "limit" matches, the weakest one is removed.
    // Time: O(limit), because at most "limit" nodes are visited. With limit = 5 this is O(1).
    void add(const Match& match)
    {
        Node* newNode = new Node(match);
        Node* previous = nullptr;
        Node* current = head;

        // Walk forward until we find a weaker match.
        while (current != nullptr && !isBetter(match, current->data))
        {
            previous = current;
            current = current->next;
        }

        newNode->next = current;

        if (previous != nullptr)
        {
            previous->next = newNode;
        }
        else
        {
            head = newNode;
        }

        count++;

        if (count > limit)
        {
            removeLast();
        }
    }

    // Gets the match at a position (0 is the best). Returns false if there is none.
    bool getAt(int position, Match& result) const
    {
        if (position < 0 || position >= count)
        {
            return false;
        }

        Node* current = head;
        for (int i = 0; i < position; i++)
        {
            current = current->next;
        }

        result = current->data;
        return true;
    }

    // Returns how many matches are stored.
    int size() const
    {
        return count;
    }
};

#endif // MATCH_LIST_H
