FROM ubuntu:xenial
MAINTAINER mail@cdroege.de

# define interface names (should be the same as in VNFD)
ENV IFIN input
ENV IFOUT output

RUN apt-get update && apt-get install -y -q \
	net-tools \
	iproute \
	inetutils-ping \
	nginx

ADD start.sh start.sh
RUN chmod +x start.sh
#ADD ipconfig.sh ipconfig.sh
#RUN chmod +x ipconfig.sh
#ADD nginx.conf /etc/nginx/nginx.conf
COPY --chown=www-data:www-data www /var/www/html

ENV SON_EMU_CMD "./start.sh"
#ENV SON_EMU_CMD_STOP "./stop.sh"

WORKDIR /

# normal entry point
CMD /bin/bash
