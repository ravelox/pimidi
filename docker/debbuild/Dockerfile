FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		autoconf \
		automake \
		build-essential \
		ca-certificates \
		dpkg-dev \
		git \
		libasound2-dev \
		libavahi-client-dev \
		libtool \
		pkg-config \
		which \
	&& rm -rf /var/lib/apt/lists/*

COPY run-debbuild.sh /usr/local/bin/run-debbuild
RUN chmod 755 /usr/local/bin/run-debbuild

USER ubuntu
WORKDIR /workspace/raveloxmidi

CMD ["/usr/local/bin/run-debbuild"]
