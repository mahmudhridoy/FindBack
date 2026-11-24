#ifndef ITEM_H
#define ITEM_H

#include <string>

using std::string;

// Stores the information submitted for a lost or found item.
struct Item
{
    int id = 0;

    string name;
    string cate;
    string description;
    string location;
    string date;
    string person;

    string type = "LOST";
    string status = "OPEN";

    bool urgent = false;
};

// Stores the matching score between a lost item and a found item.
struct Match
{
    int id = 0;
    int score = 0;

    int cat = 0;
    int kw = 0;
    int loc = 0;
    int date = 0;
};

#endif // ITEM_H
