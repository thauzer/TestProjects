#!/bin/bash
# Dumps the keys from redis database and saves the to the database. In this case the game:* and games keys.

keys=`redis-cli keys game:*`

echo ${keys}

for gamekey in ${keys}
do
        redis-cli dump ${gamekey} | head  -c -1 > ${gamekey}.dump
done

redis-cli dump games | head -c -1 > games.dump
