# generate the colours initialisation c_init.c (used in colour.c) from
# colour.d, the file of colour definitions
# in records "S msg		7 0 0 1"
# $1=type $2=name $3=fore $4=back $5=hi $6=ul
BEGIN {FS=" ";print "colour curr;"}
/^\043/ {next}
/^%/ {printf "curr.fore=%d;curr.back=%d;curr.hi=%d;curr.ul=%d; c_%s[0]=curr;c_%s[1]=curr;\n", $3, $4, $5, $6, $2, $2}
/^S/ {printf "curr.fore=%d;curr.back=%d;curr.hi=%d;curr.ul=%d; c_%s[0]=curr;\n", $3, $4, $5, $6, $2}
/^R/ {printf "curr.fore=%d;curr.back=%d;curr.hi=%d;curr.ul=%d; c_%s[1]=curr;\n", $3, $4, $5, $6, $2}
/^!/ {printf "curr.fore=%d;curr.back=%d;curr.hi=%d;curr.ul=%d; c_%s=curr;\n", $3, $4, $5, $6, $2}
