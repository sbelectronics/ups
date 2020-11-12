
#---------
# Prereqs
#---------

# apt packages
sudo apt-get install -y wiringpi python-smbus python-pip python3-setuptools
# pigpio
wget https://github.com/joan2937/pigpio/archive/master.zip -O pigpio.zip
unzip pigpio.zip
cd pigpio-master
make
sudo make install
# pip packages
sudo pip install wiringpi2
sudo pip install crc8
sudo pip install python-daemon

# ---------------------------------------------------------
# Auto-startup the daemon
# ---------------------------------------------------------

sudo cp upscontrol /usr/bin
sudo cp startup/ups.service /lib/systemd/system/
sudo chmod 644 /lib/systemd/system/ups.service
sudo systemctl daemon-reload
sudo systemctl enable ups.service

# ---------------------------------------------------------
# Setup auto-power-off
# ---------------------------------------------------------

sudo cp upscontrol /usr/bin
sudo cp shutdown/ups-shutdown.sh /lib/systemd/system-shutdown/
