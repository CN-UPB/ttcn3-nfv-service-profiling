FROM ubuntu:xenial
MAINTAINER mail@cdroege.de

# define interface names (should be the same as in VNFD)
ENV IFIN input
ENV IFOUT output

# define
ENV TCP_LISTEN_PORT 80
ENV TCP_SERVERS server 10.30.3.1:80;

RUN apt-get update && apt-get install -y -q \
	net-tools \
	iproute \
	inetutils-ping \
	nginx

ADD start.sh start.sh
RUN chmod +x start.sh
ADD nginx.conf /etc/nginx/nginx.conf

ENV SON_EMU_CMD "./start.sh"

WORKDIR /

# normal entry point
CMD /bin/bash
