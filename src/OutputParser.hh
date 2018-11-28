/*
 * Copyright 2018 Christian Dröge <mail@cdroege.de>
 *
 * All rights reserved. This program and the accompanying materials are
 * made available under the terms of the Eclipse Public License v2.0 which
 * accompanies this distribution and is available at
 *
 * http://www.eclipse.org/legal/epl-v20.html
 */

#ifndef OutputParser_HH
#define OutputParser_HH

#include <string>

namespace OutputParser {
    std::string parse(std::string input, std::string parser);

    std::string parse_wrk(std::string input, std::string parser);
    std::string parse_iperf3(std::string input, std::string parser);
}

#endif
