#ifndef OutputParser_HH
#define OutputParser_HH

#include <string>

namespace OutputParser {
    std::string parse(std::string input, std::string parser);

    std::string parse_wrk(std::string input, std::string parser);
    std::string parse_iperf3(std::string input, std::string parser);
}

#endif
