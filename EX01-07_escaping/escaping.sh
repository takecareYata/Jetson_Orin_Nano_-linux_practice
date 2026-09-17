#!/bin/bash

var=hello
echo \$var
echo \\$var
echo \"$var\"
echo \'$var\'
echo \`pwd\`
echo \a\b\c\d
echo "\a\b\c\d"

echo 111\n222\t333
echo "111\n222\t333"
echo -e "111\n222\t333"
echo -e "\x31\x32\x33"
echo -e '\x31\x32\x33'

echo $'\x31\x32\x33'

