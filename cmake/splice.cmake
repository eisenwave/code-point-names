# Splices INSERT into TEMPLATE at the marker "// AUTO-GENERATED CODE HERE",
# writing the result to OUTPUT.
# Usage:
#   cmake -DTEMPLATE=... -DINSERT=... -DOUTPUT=... -P splice.cmake
cmake_minimum_required(VERSION 3.18)
file(READ "${TEMPLATE}" tmpl)
file(READ "${INSERT}" insert)
string(REPLACE "// AUTO-GENERATED CODE HERE" "${insert}" result "${tmpl}")
file(WRITE "${OUTPUT}" "${result}")
