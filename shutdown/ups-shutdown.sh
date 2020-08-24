#!/bin/bash
echo "UPS Setting 1 second shutdown timer" > /dev/console
python /usr/bin/upscontrol -t 10 shutdown
