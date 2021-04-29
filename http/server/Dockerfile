FROM gcc

ADD server.cpp /mgt_server/
ADD Makefile /mgt_server/
ADD mgt.h /mgt_server/
ADD mgt.cpp /mgt_server/
ADD parser.cpp /mgt_server/

WORKDIR /mgt_server/

EXPOSE 12347

RUN make
