#!/bin/bash

#chown -R $EIIUID:$EIIUID /mosquitto/log
#chown -R $EIIUID:$EIIUID /mosquitto/config /mosquitto/data /run/secrets
#chmod -R 760 /tmp
#chmod -R 755 /run/secrets

exec runuser -u $EIIUSER -- $@
