FROM fedora:27

RUN dnf install -y \
    nodejs \
    git \
    make \
    gcc-c++ \
    clang \
    gnutls-devel \
    hwloc-devel \
    numactl-devel \
    boost-devel \
    cryptopp-devel \
    zlib-devel \
    xfsprogs-devel \
    lz4-devel \
    systemtap-sdt-devel \
    protobuf-devel \
    lksctp-tools-devel \
    yaml-cpp-devel \
    libaio-devel \
    libpciaccess-devel \
    libxml2-devel

RUN git clone "https://github.com/scylladb/seastar.git" -b seastar-18.08-branch --recursive /seastar-1808 && \
    git clone "https://github.com/scylladb/seastar.git" --recursive /seastar-master && \
    git clone "https://github.com/HippoBaro/ultramarine.git" /ultramarine && \
    git clone "https://github.com/mattgodbolt/compiler-explorer.git" /compiler-explorer

COPY c++.local.properties /compiler-explorer/etc/config/c++.local.properties
COPY compiler-explorer.local.properties /compiler-explorer/etc/config/compiler-explorer.local.properties
COPY start-ce.sh /start-ce.sh

RUN chmod +x start-ce.sh

RUN cd /compiler-explorer && make prereqs

EXPOSE 10240/tcp
ENTRYPOINT ["sh", "-c", "/start-ce.sh"]