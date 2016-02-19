echo "Starting Lumen Flash Sequence..."

echo "Writing Firmware..."

avrdude -c usbtiny -p t45 -U flash:w:Lumen-Arduino.ino.tiny8.hex:i 2>&1 | tee firmware.log

echo "Setting Fuses..."

avrdude -c usbtiny -p t45 -U lfuse:w:0xfe:m -U hfuse:w:0xdf:m 2>&1 | tee fuses.log

echo "\033[0;34m=========RESULT=========="
echo "\033[0;32m"
grep "bytes of flash verified" firmware.log
grep "avrdude: safemode: Fuses OK (H:FF, E:DF, L:FE)" fuses.log
echo "\033[0;31m"
grep "mismatch" firmware.log
grep "mismatch" fuses.log
echo "\033[0m"
echo "\033[0;34m=========================\033[0m"

rm firmware.log fuses.log

echo -e "\033[0;32mDone. Please check for proper verification.\033[0m"

echo -e "\033[0;32mPress any key to repeat or Ctrl-C to quit.\033[0m"
read -n 1 -s

./programLumen.sh

