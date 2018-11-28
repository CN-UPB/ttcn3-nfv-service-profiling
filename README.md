# TSP: TTCN-3-based Service Profiling for NFV Function Chains

TSP is a service profiling solution for NFV service function chains and is based on the testing language [TTCN-3](http://www.ttcn-3.org/) and the [Eclipse Titan](https://projects.eclipse.org/projects/tools.titan/) toolset. Current NFV service profiling solutions often lack interoperability with different virtualization technologies and TSP uses TTCN-3 to remedy this problem. There are three main components of TSP:

1. TTCN-3 test cases: TSP includes TTCN-3 test cases that describe an abstract NFV service profiling methodology. The methodology can be used to collect end-to-end metrics (e.g. throughput) and metrics about network functions in a service function chain (e.g. CPU usage). There is also a test case to perform performance verification of service function chains.
2. TSPF: The description format TSPF is used as the input for the test cases. A TSPF file specifies a service profiling experiment, i.e.:
   1. Parameters like vCPU count and available memory for each network function.
   2. Which program is used to create a stimulus traffic and how to extract metric values from the output of this program.
   3. Which metrics should be collected about each virtual network function (i.e. CPU usage of the virtual machine running the virtual network function).
3. TTCN-3 adapters: The test cases describe the communication with a system abstractly and adapters are needed to perform the communication with a real system. TSP uses adapters to communicate with MANO systems or systems that provide MANO-like features. There are also adapters for report generation to support different output formats.

Currently TSP has an adapter for [vim-emu](https://github.com/sonata-nfv/son-emu) as a MANO-like system and it generates CSV files for the reports, but others formats and MANO systems can be supported.

# Install

To install TSP you need to have a clean Ubuntu 16.04 installation. After this you can install TSP by following these steps:

* Install dependencies:
```
sudo apt install libxml2-dev expect sshpass
```
* Install current python ruaeml.yaml version:
```
sudo pip install ruamel.yaml
```
* Install newer boost version (>= 1.67)
```
wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
tar xvzf boost_1_67_0.tar.bz2
cd boost_1_67_0
./bootstrap.sh
sudo ./b2 -j8 install
ldconfig
```
* Install a current [cpprestsdk](https://github.com/Microsoft/cpprestsdk) version: https://github.com/Microsoft/cpprestsdk/wiki/How-to-build-for-Linux
* Install Titan binaries and add them to $PATH variable
  * Download from: https://projects.eclipse.org/projects/tools.titan/downloads
```
cd
mkdir -p titan.core/Install
mv ttcn3-6.4.pl0-linux64-gcc5.4-ubuntu16.04.tgz titan.core/Install
tar xvzf ttcn3-6.4.pl0-linux64-gcc5.4-ubuntu16.04.tgz
export PATH=$PATH:~/titan.core/Install/bin/
export TTCN3_DIR=~/titan.core/Install/
```
  * The command `compiler` should output something after this
* Install son-cli
  * Install instructions are here: https://github.com/sonata-nfv/son-cli/
  * Initialize Workspace:
```
son-workspace --init
```
  * Test:
```
son-package -h
```
* Configure Docker Engine API to listen on TCP port:
  * Edit service file:
```
$ systemctl edit docker
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -H fd:// -H tcp://127.0.0.1:2376
```
  * Restart Docker:
```
systemctl restart docker
```
* Install a vim-emu fork (only a default nfv topology file and a better WSGI framework for the DummyGateKeeper was used):
```
git clone https://github.com/cdroege/son-emu
cd ~/son-emu
docker build -t vim-emu-img .
```
* Install Agents and VNFs
```
cd misc/vim-emu/agents/ && bash build.sh
cd misc/vim-emu/vnfs/ && bash build.sh
```
* Edit src/config.cfg so that the paths are correct
* Build TSP:
```
cd teaspoon/build
make -j8
```

# Usage example

After installing TSP you can run a test case. For example, you can execute the service profiling experiment described in TSPF/TSPF_nginx_wrk.ttcn:
```
ttcn3_start tsp-profiling config.cfg TSPF_nginx_wrk.control
```

To create a new TSPF file you have to create a new file in src/TSPF/ and include it in bin/Makefile.

# Contact

Developer of TSP is Christian Dröge <mail@cdroege.de>
