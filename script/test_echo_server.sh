#!/bin/bash
set -e

RETVAL=0
IP="127.0.0.1"
PORT="2233"
MESSAGE="Hello, Echo Server!"

./build/echoserver $IP $PORT &
sleep 1

echo "Sending message to echo server: $MESSAGE"
RESPONSE=$(echo $MESSAGE | nc -w 2 $IP $PORT)

if [ "$RESPONSE" == "$MESSAGE" ]; then
    echo "Echo server is working correctly! Response: $RESPONSE"
else
    echo "Unexpected response from echo server: $RESPONSE"
    $RETVAL=1
fi

kill $!
exit $RETVAL
