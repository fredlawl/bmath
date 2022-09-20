FROM debian:buster-slim

RUN apt-get update && \
    apt-get -y install \
        g++ \
        libghc-iconv-dev \
        libreadline-dev \
        libreadline5 \
        cmake
