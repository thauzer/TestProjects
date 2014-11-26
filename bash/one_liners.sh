# this file contains just simple commands or more complex one liners

# delete files that are older than 10 days and ignore all files that contain "README"
find . -mtime +10 | xargs ls | grep -v README | xargs rm -f

# delete all files older than 10 days
find . -mtime +10 -exec rm {} \;

# get public ip
curl -s checkip.dyndns.org|sed -e 's/.*Current IP Address: //' -e 's/<.*$//'
curl ipecho.net/plain
curl ifconfig.me
wget -qO- icanhazip.com
