#!/bin/bash

sleep 1

# IP setup (we need to try different names in different scenarios, but never eth0 which is the docker if)
#ifconfig "input" down
#ifconfig "input" 10.1.0.2 netmask 255.0.0.0
#ifconfig "input" up
#
#ifconfig "output" down
#ifconfig "output" 10.2.0.1 netmask 255.0.0.0
#ifconfig "output" up
#
#
#ifconfig "mgmt" down
#ifconfig "mgmt" 200.0.0.4 netmask 255.255.255.0
#ifconfig "mgmt" up
#
#ifconfig > /ifconfig.debug
