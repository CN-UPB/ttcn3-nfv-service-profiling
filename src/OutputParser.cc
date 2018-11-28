/*
 * Copyright 2018 Christian Dröge <mail@cdroege.de>
 *
 * All rights reserved. This program and the accompanying materials are
 * made available under the terms of the Eclipse Public License v2.0 which
 * accompanies this distribution and is available at
 *
 * http://www.eclipse.org/legal/epl-v20.html
 */

#include "OutputParser.hh"
#include <TTCN3.hh>
#include <string>
#include <stdexcept>
#include <cpprest/json.h>

namespace OutputParser {

    /**
     * Parses the output of a command
     * @param input The output of the command
     * @param parser Keyword for extracting a particular metric from the input
     */
    std::string parse(std::string input, std::string parser) {
        if(parser.find("iperf3-json") == 0) {
            return parse_iperf3(input, parser);
        } else if(parser.find("wrk-json") == 0) {
            return parse_wrk(input, parser);
        } else if(parser == "null") {
            return input;
        } else {
            TTCN_error("OutputParser: received unknown parser argument");
        }
        return "";
    }

    /**
     * Parses the output of wrk with JSON output
     * @param input The output of wrk
     * @param parser Keyword for extracting a particular metric from the input
     */
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

        } else if(parser == "wrk-json-latency-99percentile") {
            if(j.at("latency_distribution").is_array()) {
                metric = j.at("latency_distribution")[3].at("latency_in_microseconds");
            }
        } else {
            TTCN_error("received unknown parser argument");
        }

        return metric.serialize();
    }

    /**
     * Parses the output of iperf3 with JSON output
     * @param input The output of iperf3
     * @param parser Keyword for extracting a particular metric from the input
     */
    std::string parse_iperf3(std::string input, std::string parser) {
        web::json::value j = web::json::value::parse(input);
        web::json::value metric;
        if(parser == "iperf3-json-bits_per_second") {
            if(j.has_field("end") && j.at("end").has_field("sum_received") && j.at("end").at("sum_received").has_field("bits_per_second")) {
                metric = j.at("end").at("sum_received").at("bits_per_second");
            } else {
                throw std::invalid_argument("Input does not contain a valid Iperf3 JSON ouput");
            }

            return metric.serialize();
        } else if(parser == "iperf3-json-mean_rtt") {
            if(j.has_field("end") && j.at("end").has_field("streams") && j.at("end").at("streams").has_field("sender") &&
                    j.at("end").at("streams").at("sender").has_field("mean_rtt")) {
                metric = j.at("end").at("streams").at("sender").at("mean_rtt");

            } else {
                TTCN_error("received unknown parser argument");
            }
        }

        return metric.serialize();
    }
}
