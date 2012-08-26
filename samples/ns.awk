#!/usr/bin/awk -f
# ns.awk
# Awk script to provide /ns, /cs and /id commands
# /ns <message> : sends message to NickServ
# /cs <message> : sends message to ChanServ
# /id <password>: identifies to NickServ with password

BEGIN {print "grab any 0 /ns"; print "grab any 0 /id"; print "grab any 0 /cs"; FS=" ";}
/\/ns/ {printf "tx %s %s :/msg -n NickServ", $2, $3; for(n=4;n<=NF;n++) printf " " $n; print "";}
/\/id/ {printf "tx %s %s :/msg -n NickServ IDENTIFY", $2, $3; for(n=4;n<=NF;n++) printf " " $n; print "";}
/\/cs/ {printf "tx %s %s :/msg -n ChanServ", $2, $3; for(n=4;n<=NF;n++) printf " " $n; print "";}
