#!/bin/bash
# deletes all the keys starting with casino:
# usage of keys can be a problem on larger databases

redis-cli KEYS "casino:*" | xargs redis-cli DEL
