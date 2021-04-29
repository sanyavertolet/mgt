FROM gcc

ADD client.cpp /mgt_client/
ADD Makefile /mgt_client/
ADD tests /mgt_client/tests

WORKDIR /mgt_client/

RUN make
