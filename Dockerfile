FROM ubuntu:20.04 AS build

ARG VERSION=1.10.0

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        wget 

RUN wget http://lloydrochester.com/code/e32-${VERSION}.tar.gz && \
    tar zxf e32-${VERSION}.tar.gz && \
    cd e32-${VERSION} && \
    ./configure && \
    make && \
    make install

FROM ubuntu:20.04

COPY --from=build /usr/local/bin/e32 /usr/local/bin/e32

ENTRYPOINT [ "/usr/local/bin/e32" ]