#include "OutputParser.hh"
#include <TTCN3.hh>
#include <string>
#include <stdexcept>
#include <cpprest/json.h>

namespace OutputParser {

    std::string parse(std::string input, std::string parser) {
        if(parser == "iperf3-json-bits_per_second") {
            return parse_iperf3(input, parser);
        } else if(parser == "wrk-json-requests") {
            return parse_wrk(input, parser);
        } else if(parser == "null") {
            return input;
        } else {
            TTCN_error("OutputParser: received unknown parser argument");
        }
        return "";
    }

    std::string parse_wrk(std::string input, std::string parser) {
        web::json::value metric;

        // Find JSON output
        std::string::size_type json_location = input.find("{");
        std::string json_data = input.substr(json_location, std::string::npos);

        web::json::value j = web::json::value::parse(json_data);

        if(parser == "wrk-json-requests") {
            if(j.has_field("requests")) {
                metric = j.at("requests");
            } else {
                throw std::invalid_argument( "Input does not contain a valid JSON ouput" );
            }

        } else if(parser == "wrk-json-bytes") {
            if(j.has_field("bytes")) {
                metric = j.at("bytes");
            } else {
                throw std::invalid_argument( "Input does not contain a valid JSON ouput" );
            }

        } else if(parser == "wrk-json-99percentile") {
            if(j.at("latency_distribution").is_array()) {
                metric = j.at("latency_distribution")[3].at("latency_in_microseconds");
            }
        } else {
            throw std::invalid_argument( "received unknown parser argument" );
        }

        return metric.serialize();
    }

    std::string parse_iperf3(std::string input, std::string parser) {
        if(parser == "iperf3-json-bits_per_second") {
            web::json::value j = web::json::value::parse(input);

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
