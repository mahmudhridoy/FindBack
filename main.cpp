// main.cpp - FindBack web server
// The Crow library answers requests from the web pages in the static folder.
// All data is kept in our own data structures (linked list, queue, stack).

#include "crow_all.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include "item.h"
#include "helpers.h"
#include "linked_list.h"
#include "match_list.h"
#include "claim_queue.h"
#include "undo_stack.h"

using namespace crow;
using std::string;
using std::to_string;
using std::ifstream;
using std::ofstream;

// ---------- Data (global so every function can use it) ----------

LinkedList items;           // every lost and found item
ClaimQueue claims(20);      // claims waiting to be processed (first come, first served)
UndoStack undoStack(50);    // the last 50 adds and deletes
int nextId = 101;           // the ID for the next new item
string lastScores = "[]";   // scores of the last matches, shown on the Admin page
string dataFile = "items.txt";

// ---------- Small helper functions ----------

// Puts quotes around text so it can be used inside JSON.
string quote(const string& text) {
    string result = "\"";

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];

        if (c == '"') {
            result += "\\\"";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n' || c == '\r' || c == '\t') {
            result += ' ';
        } else {
            result += c;
        }
    }

    result += "\"";
    return result;
}

string makeUpper(string text) {
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] >= 'a' && text[i] <= 'z') text[i] = text[i] - 32;
    }
    return text;
}

string makeLower(string text) {
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] >= 'A' && text[i] <= 'Z') text[i] = text[i] + 32;
    }
    return text;
}

// Today's date and time as text, for example "2026-11-21 14:30".
string currentTime() {
    time_t now = time(nullptr);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&now));
    return buffer;
}

// Reads a text value from the JSON the web page sent. Returns "" if it is missing.
string getText(const crow::json::rvalue& data, const char* key) {
    if (!data.has(key)) return "";
    if (data[key].t() != crow::json::type::String) return "";
    return string(data[key].s());
}

// Changes a field only when the JSON contains a new, non-empty value for it.
void updateField(string& field, const crow::json::rvalue& data, const char* key) {
    string newValue = cleanText(getText(data, key));
    if (newValue != "") field = newValue;
}

// ---------- Building JSON answers ----------

string itemToJson(const Item& item) {
    string json = "{";
    json += "\"id\":" + to_string(item.id) + ",";
    json += "\"name\":" + quote(item.name) + ",";
    json += "\"cate\":" + quote(item.cate) + ",";
    json += "\"description\":" + quote(item.description) + ",";
    json += "\"location\":" + quote(item.location) + ",";
    json += "\"date\":" + quote(item.date) + ",";
    json += "\"person\":" + quote(item.person) + ",";
    json += "\"type\":" + quote(item.type) + ",";
    json += "\"status\":" + quote(item.status) + ",";
    if (item.urgent) {
        json += "\"urgent\":true";
    } else {
        json += "\"urgent\":false";
    }
    json += "}";
    return json;
}

string claimToJson(const Claim& claim) {
    string json = "{";
    json += "\"itemId\":" + to_string(claim.itemId) + ",";
    json += "\"claimant\":" + quote(claim.claimant) + ",";
    json += "\"time\":" + quote(claim.time);
    json += "}";
    return json;
}

response sendJson(int code, const string& body) {
    response result(code, body);
    result.set_header("Content-Type", "application/json");
    return result;
}

response sendError(int code, const string& message) {
    return sendJson(code, "{\"error\":" + quote(message) + "}");
}

// ---------- Saving and loading items.txt ----------

// Writes every item to the file, one item per line, fields separated by tabs.
void saveItems() {
    ofstream file(dataFile);

    Item* item = items.first();
    while (item != nullptr) {
        file << item->id << '\t' << item->name << '\t' << item->cate << '\t'
             << item->description << '\t' << item->location << '\t' << item->date << '\t'
             << item->person << '\t' << item->type << '\t' << item->status << '\t';

        if (item->urgent) {
            file << 1;
        } else {
            file << 0;
        }
        file << '\n';

        item = items.next();
    }
}

// Splits one line at every tab. Returns how many parts were found.
int splitByTab(const string& line, string parts[], int maxParts) {
    int count = 0;
    string current = "";

    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '\t') {
            if (count < maxParts) {
                parts[count] = current;
                count++;
            }
            current = "";
        } else {
            current += line[i];
        }
    }

    if (count < maxParts) {
        parts[count] = current;
        count++;
    }

    return count;
}

// Reads items.txt when the server starts.
void loadItems() {
    ifstream file(dataFile);
    string line;

    while (getline(file, line)) {
        string parts[10];
        int partCount = splitByTab(line, parts, 10);

        if (partCount < 10) {
            continue;   // skip a broken line
        }

        Item item;
        item.id = atoi(parts[0].c_str());
        item.name = parts[1];
        item.cate = parts[2];
        item.description = parts[3];
        item.location = parts[4];
        item.date = parts[5];
        item.person = parts[6];
        item.type = parts[7];
        item.status = parts[8];
        item.urgent = (parts[9] == "1");

        if (item.id > 0 && items.insertAtEnd(item)) {
            if (item.id >= nextId) nextId = item.id + 1;
        }
    }
}

// ---------- Matching: how well does a found item fit a lost item? ----------

// Turns a date like 2026-11-21 into a number so two dates can be compared.
int dayNumber(const string& date) {
    int year = 0;
    int month = 0;
    int day = 0;

    if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3) return -1;
    return year * 372 + month * 31 + day;
}

// Some campus places are close to each other.
bool isNearby(const string& a, const string& b) {
    if ((a == "Library" && b == "Main Building") || (a == "Main Building" && b == "Library")) {
        return true;
    }
    if ((a == "Cafeteria" && b == "Main Building") || (a == "Main Building" && b == "Cafeteria")) {
        return true;
    }
    if ((a == "Sports Ground" && b == "Dormitory") || (a == "Dormitory" && b == "Sports Ground")) {
        return true;
    }
    return false;
}

// Gives the found item a score out of 100 for the lost item.
Match calculateScore(const Item& lost, string lostWords[], int lostWordCount, const Item& found) {
    Match match;
    match.id = found.id;

    // Same category: 30 points.
    if (lost.cate != "" && lost.cate == found.cate) match.cat = 30;

    // Words that appear in both names/descriptions: 10 points each, at most 30.
    string foundWords[64];
    int foundWordCount = tokenize(found.name + " " + found.description, foundWords, 64);
    int sharedWords = 0;

    for (int i = 0; i < lostWordCount; i++) {
        for (int j = 0; j < foundWordCount; j++) {
            if (lostWords[i] == foundWords[j]) {
                sharedWords++;
                break;
            }
        }
    }

    match.kw = sharedWords * 10;
    if (match.kw > 30) match.kw = 30;

    // Same place: 25 points. A nearby place: 10 points.
    if (lost.location == found.location) {
        match.loc = 25;
    } else if (isNearby(lost.location, found.location)) {
        match.loc = 10;
    }

    // Dates close together: 15, 10 or 5 points.
    int lostDay = dayNumber(lost.date);
    int foundDay = dayNumber(found.date);

    if (lostDay >= 0 && foundDay >= 0) {
        int difference = lostDay - foundDay;
        if (difference < 0) difference = -difference;

        if (difference <= 3) {
            match.date = 15;
        } else if (difference <= 7) {
            match.date = 10;
        } else if (difference <= 30) {
            match.date = 5;
        }
    }

    match.score = match.cat + match.kw + match.loc + match.date;
    return match;
}

// Adds one part of the score to the two texts that are sent back to the web page.
void addScorePart(string& breakdown, string& reason, const string& name, int points) {
    if (points == 0) return;

    if (breakdown != "") {
        breakdown += ",";
        reason += ", ";
    }

    breakdown += "\"" + name + "\":" + to_string(points);
    reason += name + " +" + to_string(points);
}

// ---------- Route functions (one for each part of the website) ----------

// GET /api/stats - numbers for the dashboard
response getStats() {
    int lost = 0;
    int found = 0;
    int urgent = 0;

    Item* item = items.first();
    while (item != nullptr) {
        if (item->type == "LOST") {
            lost++;
        } else {
            found++;
        }

        if (item->urgent && item->status == "OPEN") urgent++;

        item = items.next();
    }

    string json = "{";
    json += "\"total\":" + to_string(items.size()) + ",";
    json += "\"lost\":" + to_string(lost) + ",";
    json += "\"found\":" + to_string(found) + ",";
    json += "\"claims\":" + to_string(claims.size()) + ",";
    json += "\"urgent\":" + to_string(urgent);
    json += "}";
    return sendJson(200, json);
}

// GET /api/items - list every item. POST /api/items - add a new item.
response handleItems(const request& req) {
    if (req.method == "GET"_method) {
        string json = "[";
        Item* item = items.first();

        while (item != nullptr) {
            json += itemToJson(*item);
            item = items.next();
            if (item != nullptr) json += ",";
        }

        json += "]";
        return sendJson(200, json);
    }

    // POST: read the new item from the JSON sent by the report page.
    crow::json::rvalue data = crow::json::load(req.body);
    if (!data) return sendError(400, "Request body must be valid JSON");

    Item item;
    item.name = cleanText(getText(data, "name"));
    item.cate = cleanText(getText(data, "cate"));
    item.description = cleanText(getText(data, "description"));
    item.location = cleanText(getText(data, "location"));
    item.date = cleanText(getText(data, "date"));
    item.person = cleanText(getText(data, "person"));
    item.type = makeUpper(getText(data, "type"));
    item.urgent = data.has("urgent") && data["urgent"].t() == crow::json::type::True;

    if (item.name == "") return sendError(400, "Item name is required");
    if (item.person == "") return sendError(400, "Your name or contact is required");
    if (item.type != "LOST" && item.type != "FOUND") {
        return sendError(400, "Type must be LOST or FOUND");
    }

    item.id = nextId;
    nextId++;

    if (!items.insert(item)) return sendError(409, "Duplicate item ID " + to_string(item.id));

    // Remember this add so the admin can undo it.
    Action action;
    action.kind = "ADD";
    action.item = item;
    action.desc = "Add item " + to_string(item.id) + " (" + item.name + ")";
    undoStack.push(action);

    saveItems();
    return sendJson(201, itemToJson(item));
}

// GET, PUT or DELETE /api/items/<id> - work with one item
response handleOneItem(const request& req, int id) {
    Item* item = items.find(id);
    if (item == nullptr) return sendError(404, "Item " + to_string(id) + " not found");

    if (req.method == "GET"_method) return sendJson(200, itemToJson(*item));

    if (req.method == "PUT"_method) {
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data) return sendError(400, "Request body must be valid JSON");

        updateField(item->name, data, "name");
        updateField(item->cate, data, "cate");
        updateField(item->description, data, "description");
        updateField(item->location, data, "location");
        updateField(item->date, data, "date");
        updateField(item->person, data, "person");

        string status = makeUpper(getText(data, "status"));
        if (status == "OPEN" || status == "CLAIMED") item->status = status;
        if (data.has("urgent")) item->urgent = (data["urgent"].t() == crow::json::type::True);

        saveItems();
        return sendJson(200, itemToJson(*item));
    }

    // DELETE: keep a copy first so the delete can be undone.
    Action action;
    action.kind = "DELETE";
    action.item = *item;
    action.desc = "Delete item " + to_string(id) + " (" + item->name + ")";

    items.remove(id);
    undoStack.push(action);

    saveItems();
    return sendJson(200, "{\"deleted\":" + to_string(id) + "}");
}

// GET /api/search?q=word - look at every item and keep the ones that contain the words
response searchItems(const request& req) {
    const char* queryPointer = req.url_params.get("q");
    string query = "";
    if (queryPointer != nullptr) query = queryPointer;

    string words[16];
    int wordCount = tokenize(query, words, 16);

    string json = "{\"words\":[],\"items\":[";

    if (wordCount > 0) {
        bool firstResult = true;
        Item* item = items.first();

        while (item != nullptr) {
            string text = makeLower(item->name + " " + item->cate + " " + item->description + " " + item->location);

            bool allWordsFound = true;
            for (int i = 0; i < wordCount; i++) {
                if (text.find(words[i]) == string::npos) allWordsFound = false;
            }

            if (allWordsFound) {
                if (!firstResult) json += ",";
                json += itemToJson(*item);
                firstResult = false;
            }

            item = items.next();
        }
    }

    json += "]}";
    return sendJson(200, json);
}

// GET /api/matches/<id> - the best 5 found items for one lost item
response findMatches(int id) {
    Item* lost = items.find(id);
    if (lost == nullptr) return sendError(404, "Item " + to_string(id) + " not found");
    if (lost->type != "LOST") return sendError(400, "Matches can only be found for a LOST item");

    string lostWords[64];
    int lostWordCount = tokenize(lost->name + " " + lost->description, lostWords, 64);

    // Score every open found item. The sorted list keeps only the best 5.
    MatchList bestMatches(5);
    Item* found = items.first();

    while (found != nullptr) {
        if (found->type == "FOUND" && found->status == "OPEN") {
            Match match = calculateScore(*lost, lostWords, lostWordCount, *found);
            if (match.score >= 30) bestMatches.add(match);
        }
        found = items.next();
    }

    // Turn the list into JSON.
    string json = "[";
    lastScores = "[";

    for (int i = 0; i < bestMatches.size(); i++) {
        Match match;
        bestMatches.getAt(i, match);
        Item* foundItem = items.find(match.id);

        if (i > 0) {
            json += ",";
            lastScores += ",";
        }
        lastScores += to_string(match.score);

        string breakdown = "";
        string reason = "";
        addScorePart(breakdown, reason, "category", match.cat);
        addScorePart(breakdown, reason, "keywords", match.kw);
        addScorePart(breakdown, reason, "location", match.loc);
        addScorePart(breakdown, reason, "date", match.date);

        json += "{";
        json += "\"id\":" + to_string(foundItem->id) + ",";
        json += "\"name\":" + quote(foundItem->name) + ",";
        json += "\"location\":" + quote(foundItem->location) + ",";
        json += "\"score\":" + to_string(match.score) + ",";
        json += "\"reason\":" + quote(reason) + ",";
        json += "\"breakdown\":{" + breakdown + "}";
        json += "}";
    }

    json += "]";
    lastScores += "]";
    return sendJson(200, json);
}

// GET /api/claims - show the queue. POST /api/claims - add a claim to the queue.
response handleClaims(const request& req) {
    if (req.method == "GET"_method) {
        string json = "{";
        json += "\"size\":" + to_string(claims.size()) + ",";
        json += "\"capacity\":" + to_string(claims.capacity()) + ",";
        json += "\"items\":[";

        for (int i = 0; i < claims.size(); i++) {
            if (i > 0) json += ",";
            json += claimToJson(claims.at(i));
        }

        json += "]}";
        return sendJson(200, json);
    }

    // POST
    crow::json::rvalue data = crow::json::load(req.body);
    if (!data) return sendError(400, "Request body must be valid JSON");
    if (!data.has("itemId") || data["itemId"].t() != crow::json::type::Number) {
        return sendError(400, "Item ID is required");
    }

    Claim claim;
    claim.itemId = (int)data["itemId"].i();
    claim.claimant = cleanText(getText(data, "claimant"));
    claim.time = currentTime();

    if (claim.claimant == "") return sendError(400, "Claimant name is required");

    Item* item = items.find(claim.itemId);
    if (item == nullptr) return sendError(404, "Item " + to_string(claim.itemId) + " not found");
    if (item->status == "CLAIMED") {
        return sendError(409, "Item " + to_string(claim.itemId) + " is already claimed");
    }

    if (!claims.enqueue(claim)) {
        return sendError(409, "Claim queue is full (" + to_string(claims.capacity()) + "). Process a claim first.");
    }

    return sendJson(201, claimToJson(claim));
}

// POST /api/claims/next - take the first claim out of the queue
response processNextClaim() {
    Claim claim;

    if (!claims.dequeue(claim)) return sendError(409, "The claim queue is empty");

    Item* item = items.find(claim.itemId);
    if (item != nullptr) {
        item->status = "CLAIMED";

        // Remember this claim so the admin can undo it.
        Action action;
        action.kind = "CLAIM";
        action.item = *item;
        action.claimant = claim.claimant;
        action.claimTime = claim.time;
        action.desc = "Process claim for item " + to_string(item->id) + " (" + item->name + ")";
        undoStack.push(action);

        saveItems();
    }

    return sendJson(200, claimToJson(claim));
}

// GET /api/undo - show the history. POST /api/undo - undo the newest action.
response handleUndo(const request& req) {
    if (req.method == "GET"_method) {
        string json = "[";

        for (int i = 0; i < undoStack.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"desc\":" + quote(undoStack.fromTop(i).desc) + "}";
        }

        json += "]";
        return sendJson(200, json);
    }

    // POST: take the newest action off the stack and reverse it.
    Action action;
    if (!undoStack.pop(action)) return sendError(409, "Nothing to undo");

    if (action.kind == "ADD") {
        items.remove(action.item.id);
    } else if (action.kind == "DELETE") {
        items.insert(action.item);
    } else if (action.kind == "CLAIM") {
        // Open the item again and put the claim back at the front of the queue.
        Item* item = items.find(action.item.id);
        if (item != nullptr) {
            item->status = "OPEN";

            Claim claim;
            claim.itemId = action.item.id;
            claim.claimant = action.claimant;
            claim.time = action.claimTime;
            claims.addToFront(claim);
        }
    }

    saveItems();
    return sendJson(200, "{\"desc\":" + quote(action.desc) + "}");
}

// GET /api/debug/structures - what is inside the data structures (for the demo)
response getStructures() {
    string latestUndo = "";
    if (undoStack.size() > 0) latestUndo = undoStack.fromTop(0).desc;

    string json = "{";
    json += "\"linkedList\":{\"size\":" + to_string(items.size()) + "},";
    json += "\"matchList\":{\"lastMatchScores\":" + lastScores + "},";
    json += "\"queue\":{";
    json += "\"size\":" + to_string(claims.size()) + ",";
    json += "\"capacity\":" + to_string(claims.capacity()) + ",";
    json += "\"frontIndex\":" + to_string(claims.frontIndex()) + ",";
    json += "\"rearIndex\":" + to_string(claims.rearIndex()) + "},";
    json += "\"undoStack\":{\"size\":" + to_string(undoStack.size()) + ",\"top\":" + quote(latestUndo) + "}";
    json += "}";
    return sendJson(200, json);
}

// ---------- Start the server ----------

int main() {
    loadItems();

    SimpleApp app;

    // Open the dashboard when someone visits the main address.
    CROW_ROUTE(app, "/")([]() {
        response page;
        page.redirect("/static/index.html");
        return page;
    });

    // Each route below just calls one of the functions above.
    CROW_ROUTE(app, "/api/stats")([]() { return getStats(); });

    CROW_ROUTE(app, "/api/items").methods("GET"_method, "POST"_method)
    ([](const request& req) { return handleItems(req); });

    CROW_ROUTE(app, "/api/items/<int>").methods("GET"_method, "PUT"_method, "DELETE"_method)
    ([](const request& req, int id) { return handleOneItem(req, id); });

    CROW_ROUTE(app, "/api/search")
    ([](const request& req) { return searchItems(req); });

    CROW_ROUTE(app, "/api/matches/<int>")
    ([](int id) { return findMatches(id); });

    CROW_ROUTE(app, "/api/claims").methods("GET"_method, "POST"_method)
    ([](const request& req) { return handleClaims(req); });

    CROW_ROUTE(app, "/api/claims/next").methods("POST"_method)
    ([]() { return processNextClaim(); });

    CROW_ROUTE(app, "/api/undo").methods("GET"_method, "POST"_method)
    ([](const request& req) { return handleUndo(req); });

    CROW_ROUTE(app, "/api/debug/structures")
    ([]() { return getStructures(); });

    app.port(18080).run();
}
