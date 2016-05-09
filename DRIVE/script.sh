#!/bin/bash
COUNTER=1
while [ $COUNTER -lt 33 ]; do
    touch SEGMENT$COUNTER
    let COUNTER=COUNTER+1
done
