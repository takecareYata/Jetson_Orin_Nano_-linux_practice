#!/bin/bash

name="linux"
course="expert"

# linux expert
var="$name $course"
echo $var

# $linux \expert
var="\$$name \\$course"
echo $var

# $$linux \\expert
var="\$\$$name \\\\$course"
echo $var

# experted linux
var="${course}ed $name"
#var=""$course"ed $name"
echo $var
