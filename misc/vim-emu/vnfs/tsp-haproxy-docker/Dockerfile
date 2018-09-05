FROM ubuntu:xenial
MAINTAINER mail@cdroege.de

ENV HTTP_SERVER 10.30.1.1

RUN	apt-get update && apt-get install -y \
	net-tools \
	iproute \
	inetutils-ping \
	haproxy

ADD start.sh start.sh
RUN chmod +x start.sh
ADD ipconfig.sh ipconfig.sh
RUN chmod +x ipconfig.sh
COPY haproxy.cfg /etc/haproxy/haproxy.cfg

# set entry point for emulator gatekeeper
ENV SON_EMU_CMD "./start.sh"

CMD /bin/bash
