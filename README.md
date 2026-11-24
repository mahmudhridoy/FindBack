# FindBack
FindBack is a web-based lost and found system for campus, where reports are stored, matched and claimed using data structures we built ourselves in C++.

FindBack helps people get lost items back to their owners. Users report items they lost or found, search every report in one place, and see a ranked list of the best matches for a lost item. Claims are handled in the order they arrive, and an admin can undo mistakes. The server is written in C++ with the Crow framework. The data structures behind it (a singly linked list, a sorted linked list, a circular queue and a stack) are our own implementations with no STL containers. The website works on both computers and phones.

Header files:
- item.h: Item and Match data models
- helpers.h: text cleaning and tokenization helpers
- linked_list.h: singly linked list used for all item records
- match_list.h: small sorted linked list that keeps the 5 best matches
- claim_queue.h: circular FIFO queue for pending claims
- undo_stack.h: LIFO stack for undo history

Item ID lookup and keyword search walk through the singly linked list (O(n)).
The server is single-threaded and uses plain functions, so it needs no locks.
Matching scores every open found item and inserts it into a sorted linked list that keeps only the best 5.

Build (Windows / MinGW):
`g++ -std=c++17 -O2 -pthread -I. main.cpp -o findback.exe -lws2_32 -lmswsock`

Keep crow_all.h and its required Asio files in this folder. Run from this
folder so static/ and items.txt resolve correctly, then open:
http://localhost:18080
