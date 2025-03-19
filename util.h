//
// Created by Raunaq Pahwa on 3/1/25.
//

#ifndef SCHEDULER_UTIL_H
#define SCHEDULER_UTIL_H

auto SplitString(const std::string& str, char delimiter) -> std::vector<std::string> {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream input_stream(str);
    while (std::getline(input_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

#endif //SCHEDULER_UTIL_H
