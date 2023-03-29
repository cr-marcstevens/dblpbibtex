FROM debian:stable


# environment variables and arguments
ENV SOURCE_URL https://github.com/cr-marcstevens/dblpbibtex
ENV WORKDIR /workdir
ARG UID 1000
ARG GUID 1000



# install compilation dependencies
RUN apt-get update && apt-get install -y -q --no-install-recommends libboost-dev git build-essential dh-autoreconf libcurl4-gnutls-dev texlive

# reinstall certificates (to avoid further errors)
RUN apt-get install --reinstall ca-certificates -y


# clone repository
RUN cd /tmp && git clone $SOURCE_URL

# compile and install dblpbibtex
RUN cd /tmp/dblpbibtex && autoreconf --install && ./configure && make && make install


# set the working directory
WORKDIR $WORKDIR

# switch user
USER ${UID}:${GUID}


