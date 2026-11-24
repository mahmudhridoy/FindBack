#ifndef HELPERS_H
#define HELPERS_H

#include <string>

using std::string;

// Replaces tabs and line breaks with spaces before saving submitted text.
inline string cleanText(string text)
{
    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] == '\t' || text[i] == '\n' || text[i] == '\r')
        {
            text[i] = ' ';
        }
    }

    return text;
}

// Splits text into unique lowercase words.
// Words shorter than two characters are ignored.
// The function stores no more than maxN words in the output array.
inline int tokenize(const string& text, string* output, int maxN)
{
    int wordCount = 0;
    string currentWord;

    for (size_t i = 0; i <= text.size(); i++)
    {
        char currentChar = (i < text.size()) ? text[i] : ' ';

        if (currentChar >= 'A' && currentChar <= 'Z')
        {
            currentChar = static_cast<char>(currentChar - 'A' + 'a');
        }

        if ((currentChar >= 'a' && currentChar <= 'z') ||
            (currentChar >= '0' && currentChar <= '9'))
        {
            currentWord += currentChar;
        }
        else
        {
            if (currentWord.size() >= 2 && wordCount < maxN)
            {
                bool alreadyExists = false;

                for (int k = 0; k < wordCount; k++)
                {
                    if (output[k] == currentWord)
                    {
                        alreadyExists = true;
                        break;
                    }
                }

                if (!alreadyExists)
                {
                    output[wordCount] = currentWord;
                    wordCount++;
                }
            }

            currentWord.clear();
        }
    }

    return wordCount;
}

#endif // HELPERS_H
