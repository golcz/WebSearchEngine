#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

class Utils 
{
public:
    /**
     * @brief Tokenizes the input text into words.
     *
     * @param text The input text to tokenize.
     * @return A vector of tokenized words.
     */
    static std::vector<std::string> tokenize(const std::string &text);

    /**
     * @brief Encodes a number using varbyte encoding and writes it to an output stream.
     *
     * @param outStream The output stream to write the encoded number to.
     * @param num The number to encode.
     */
    static void varbyteEncode(std::ostream &outStream, uint32_t num);

    /**
     * @brief Decodes a varbyte-encoded number from the input stream.
     *
     * @param inStream The input stream to read the encoded number from.
     * @return The decoded number.
     */
    static uint32_t varbyteDecode(std::istream &inStream);
};

#endif //UTILS_H
