curl -i -X POST -F package=@/home/dark/son-examples/service-projects/sonata-snort-service.son http://172.17.0.2:5000/packages
curl -X POST http://172.17.0.2:5000/instantiations -d "{}"
docker exec vim-emu vim-emu compute start -d dc1 -n server -i iperf3-ssh
docker exec vim-emu vim-emu compute start -d dc1 -n client -i iperf3-ssh
docker exec vim-emu vim-emu network add -b -src client:client-eth0 -dst snort_vnf:input
docker exec vim-emu vim-emu network add -b -src snort_vnf:output -dst server:server-eth0
#docker exec vim-emu vim-emu compute stop -d dc1  -n server
#docker exec vim-emu vim-emu compute stop -d dc1  -n client
