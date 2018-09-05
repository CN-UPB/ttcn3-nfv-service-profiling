#!/bin/bash
./ipconfig.sh > /mnt/share/ipconfig.log

sed -i 's/HTTP_SERVER/'"$HTTP_SERVER"'/g' /etc/haproxy/haproxy.cfg

service haproxy restart
