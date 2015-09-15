#!/bin/bash
# Restores keys from files that were created by redis_dump.sh script. 

files=`ls game*`

echo ${files}

echo "keys"

for gamefile in ${files}
do
        key=${gamefile%\.dump}
        echo ${key}
        cat ${gamefile} | redis-cli -x restore ${key} 0
done
