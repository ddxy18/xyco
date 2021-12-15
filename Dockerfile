# syntax=docker/dockerfile:1
FROM ddxy18/dev-env:latest

SHELL ["zsh", "-c"]
# install liburing
RUN apt-get install -y gcc \
    && cd $HOME && git clone https://github.com/axboe/liburing.git && cd liburing && ./configure && make install && make clean \
    && cd $HOME && git clone https://github.com/ddxy18/xyco.git && cd xyco && scripts/setup.sh \
    && apt-get remove -y gcc && apt-get autoremove -y

CMD [ "zsh" ]