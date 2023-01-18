
mount='--mount type=bind,source=/home,target=/home'
net='--net anet'
img="centos8-vxr"
bin="/usr/bin/ms_server"
home="/home/chris"
cfg="-d ${home}/raims_test/run/vxr_mesh.yaml"
mkdir -p ${home}/raims_test/log

for host in vxr-prod-oms01 vxr-prod-oms02 vxr-prod-oms03 vxr-prod-oms04 vxr-prod-tsm01 vxr-prod-tsm02 vxr-prod-qs01 vxr-prod-qs02 vxr-prod-lm01 vxr-prod-lm02 vxr-prod-strategy01 vxr-prod-strategy02 vxr-prod-strategy03 vxr-prod-rundeck vxr-prod-mqscript ; do
sudo docker run -d --name ${host} --hostname ${host} ${net} ${mount} ${img} ${bin} ${cfg} -l ${home}/raims_test/log/${host}.log
done

cfg="-d ${home}/raims_test/run/vxr_mesh_raicache.yaml"
for host in vxr-prod-procs01 vxr-prod-procs02 ; do
sudo docker run -d --name ${host} --hostname ${host} ${net} ${mount} ${img} ${bin} ${cfg} -l ${home}/raims_test/log/${host}.log
done

cfg="-d ${home}/raims_test/run/raicache_rv.yaml"
for host in vxr-prod-raicache01 vxr-prod-raicache02 ; do
sudo docker run -d --name ${host} --hostname ${host} ${net} ${mount} ${img} ${bin} ${cfg} -l ${home}/raims_test/log/${host}.log
done
