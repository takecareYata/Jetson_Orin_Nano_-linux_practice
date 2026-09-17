#!/bin/bash

var=hello

touch aaa bbb
touch "aaa bbb"

echo $var
echo "$var"

ls /etc/issue*
ls "/etc/issue*"

touch 'ccc ddd'
echo '$var'
ls '/etc/issue*'

