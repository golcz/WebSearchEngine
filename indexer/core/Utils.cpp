#include "Utils.h"
#include <cctype>

using namespace std;

/**
 * @brief Tokenizes the input text into words.
 *
 * @param text The input text to tokenize.
 * @return A vector of tokenized words.
 */
vector<string> Utils::tokenize(const string &text) 
{
    vector<string> tokens;
    string word;

    // Loop through each character in the input text
    for (char ch : text) 
    {
        if (isalnum(ch)) 
        {
            word += tolower(ch);  // Add lowercase alphanumeric characters to word
        } 
        else if (!word.empty()) 
        {
            tokens.push_back(word);  // Save the word when a non-alphanumeric character is encountered
            word.clear();  // Clear the word to start forming the next one
        }
    }

    // Add the last word if there's any remaining
    if (!word.empty()) 
    {
        tokens.push_back(word);
    }

    return tokens;
}

/**
 * @brief Encodes a number using varbyte encoding and writes it to an output stream.
 *
 * @param outStream The output stream to write the encoded number to.
 * @param num The number to encode.
 */
void Utils::varbyteEncode(ostream &outStream, uint32_t num) 
{
    // Use varbyte encoding: Split number into 7-bit chunks and write to the stream
    while (num >= 128) 
    {  // As long as the number exceeds 7 bits, keep extracting 7 bits
        outStream.put(static_cast<char>((num & 0x7F) | 0x80));  // Set continuation bit
        num >>= 7;  // Shift right by 7 bits
    }
    outStream.put(static_cast<char>(num));  // Final byte without the continuation bit
}

/**
 * @brief Decodes a varbyte-encoded number from the input stream.
 *
 * @param inStream The input stream to read the encoded number from.
 * @return The decoded number.
 */
uint32_t Utils::varbyteDecode(istream &inStream) 
{
    uint32_t num = 0;
    int shift = 0;
    char byte;

    while (inStream.get(byte)) 
    {
        num |= (byte & 0x7F) << shift;  // Add the 7-bit value to the number
        if ((byte & 0x80) == 0) {  // If the continuation bit is 0, stop
            break;
        }
        shift += 7;
    }

    return num;
}
