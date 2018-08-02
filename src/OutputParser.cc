#include "OutputParser.hh"
#include <string>
#include <stdexcept>
#include <cpprest/json.h>

namespace OutputParser {

    std::string parse(std::string input, std::string parser) {
        if(parser == "iperf3-json-bits_per_second") {
            return parse_iperf3(input, parser);
        } else {
            throw std::invalid_argument( "received unknown parser argument" );
        }
    }

    std::string parse_iperf3(std::string input, std::string parser) {
        if(parser == "iperf3-json-bits_per_second") {
            web::json::value j = web::json::value::parse(input);

            std::cout << j << "\n";

            web::json::value metric;

            if(j.has_field("end") && j.at("end").has_field("sum_received") && j.at("end").at("sum_received").has_field("bits_per_second")) {
                metric = j.at("end").at("sum_received").at("bits_per_second");
            } else {
                throw std::invalid_argument( "Input does not contain a valid Iperf3 JSON ouput" );
            }

            return metric.serialize();
        } else {
            throw std::invalid_argument( "received unknown parser argument" );
        }
    }

}
