# download the latest development packages
pip3 install "amaranth[builtin-yosys] @ git+https://github.com/amaranth-lang/amaranth.git"
pip3 install "amaranth-boards[builtin-yosys] @ git+https://github.com/amaranth-lang/amaranth-boards.git"

# download the dependencies
sudo apt-get install yosys dfu-util nextpnr-ice40 

# icestorm is not packaged very often
trap 'rm -rf /tmp/icestorm' INT TERM EXIT HUP
git clone https://github.com/cliffordwolf/icestorm.git /tmp/icestorm
make -C /tmp/icestorm
sudo make -C /tmp/icestorm install
