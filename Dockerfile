# syntax=docker/dockerfile:1
FROM ddxy18/dev-env:latest

SHELL ["zsh", "-c"]
# install liburing
RUN apt-get install liburing-dev \
    && cd $HOME && git clone https://github.com/ddxy18/xyco.git && cd xyco && scripts/setup.sh

CMD [ "zsh" ]