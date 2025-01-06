FROM alpine:3.21

LABEL maintainer="Bertrik Sikken bertrik@gmail.com"

RUN apk add python3 py-requests py-configargparse

ADD gitlabstatuslight.py /opt

WORKDIR /opt
ENTRYPOINT ["/bin/sh"]
CMD ["-c", "./gitlabstatuslight.py -gitlab ${GITLAB_HOST} -t ${GITLAB_TOKEN} -patlite ${PATLITE_HOST}"]

