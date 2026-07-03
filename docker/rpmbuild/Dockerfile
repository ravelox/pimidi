FROM fedora:44

RUN dnf -y install \
	alsa-lib-devel \
	autoconf \
	automake \
	avahi-devel \
	diffutils \
	findutils \
	gcc \
	git \
	gzip \
	libtool \
	make \
	pkgconf-pkg-config \
	rpm-build \
	sed \
	tar \
	which \
	&& dnf clean all

RUN useradd --create-home --shell /bin/bash builder

COPY run-rpmbuild.sh /usr/local/bin/run-rpmbuild
RUN chmod 755 /usr/local/bin/run-rpmbuild

USER builder
WORKDIR /workspace/raveloxmidi

CMD ["/usr/local/bin/run-rpmbuild"]
