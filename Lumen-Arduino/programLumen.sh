#!/bin/bash

# Color definitions
BLACK='\033[;30m'
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
YELLOW='\033[1;33m'
WHITE='\033[1;37m'
NC='\033[0m'


# Configuration
dir=$(dirname $0)
avrdude="avrdude"
programmer="usbtiny"
microcontroller="t45"
hexfile="$dir/Lumen-Arduino.ino.tiny8.hex"
logfile="$dir/programlumen.log"

# Flash Thruster Commander
echo "Starting Lumen Flash Sequence..."

#Write program and set fuses
$avrdude -c$programmer -p$microcontroller -e -Uefuse:w:0xff:m -Uhfuse:w:0xdf:m -Ulfuse:w:0xfe:m -Uflash:w:$hexfile -u 2>&1 | tee $logfile

echo -e "${BLUE}=========RESULT=========="
echo -e "${GREEN}"
grep "bytes of efuse verified" $logfile
grep "bytes of hfuse verified" $logfile
grep "bytes of lfuse verified" $logfile
grep "bytes of flash verified" $logfile
echo -e "${RED}"
grep "mismatch" $logfile
echo -e "${NC}"
echo -e "${BLUE}========================="
echo -e "${NC}"

rm $logfile

echo -e "${GREEN}Done. Please check for proper verification.${NC}"

echo -e "${GREEN}Press any key to repeat or Ctrl-C to quit.${NC}"
read -n 1 -s

$0
