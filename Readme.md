# Install

Here are the steps to run a TSP service profiling test case:

* Prerequisite: Ubuntu 16.04
* Install dependencies:
```sudo apt install libcpprest-dev libxml2-dev expect sshpass
```
* Install current python ruaeml.yaml version:
```sudo pip install ruamel.yaml
```
* Install newer boost version (>= 1.67)
```wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2
tar xvzf boost_1_67_0.tar.bz2
cd boost_1_67_0
./bootstrap.sh
sudo ./b2 -j8 install```
* Install Titan binaries and add them to $PATH variable
** Download from: https://projects.eclipse.org/projects/tools.titan/downloads
```
cd
mkdir -p titan.core/Install
mv ttcn3-6.4.pl0-linux64-gcc5.4-ubuntu16.04.tgz titan.core/Install
tar xvzf ttcn3-6.4.pl0-linux64-gcc5.4-ubuntu16.04.tgz
export PATH=$PATH:~/titan.core/Install/bin/
export TTCN3_DIR=~/titan.core/Install/
```
** The command `compiler` should output something after this
* Install son-cli
** Install instructions are here: https://github.com/sonata-nfv/son-cli/
** Initalize Workspace:
```son-workspace --init```
** Test:
```son-package -h```
* Configure Docker Engine API to listen on TCP port:
** Edit service file:
```$ systemctl edit docker
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -H fd:// -H tcp://127.0.0.1:2376```
** Restart Docker:
```systemctl restart docker```
* Install a vim-emu fork (only a default nfv topology file and a better WSGI framework for the DummyGateKeeper was used):
```git clone https://github.com/cdroege/son-emu
cd ~/son-emu
docker build -t vim-emu-img .
```
* Install Agents and VNFs
```cd misc/vim-emu/agents/ && bash build.sh
cd misc/vim-emu/vnfs/ && bash build.sh```
* Edit src/config.cfg
* Build and start test executable:
```cd teaspoon/build
make -j8 && ttcn3_start tsp-profiling config.cfg TSPF_nginx_wrk.control```