#!/bin/bash

# modify config using ENV vars
sed -i 's/TCP_LISTEN_PORT/'"$TCP_LISTEN_PORT"'/g' /etc/nginx/nginx.conf
sed -i 's/TCP_SERVERS/'"$TCP_SERVERS"'/g' /etc/nginx/nginx.conf

service nginx restart

echo "Nginx VNF started ..."



