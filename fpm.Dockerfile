FROM debian:bullseye-slim

RUN apt-get update && \
    apt-get -y install \
        binutils \
        rubygems \
        squashfs-tools && \
    gem install fpm
