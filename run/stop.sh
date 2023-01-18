
for host in vxr-prod-oms01 vxr-prod-oms02 vxr-prod-oms03 vxr-prod-oms04 vxr-prod-tsm01 vxr-prod-tsm02 vxr-prod-qs01 vxr-prod-qs02 vxr-prod-lm01 vxr-prod-lm02 vxr-prod-strategy01 vxr-prod-strategy02 vxr-prod-strategy03 vxr-prod-rundeck vxr-prod-mqscript ; do
sudo docker stop $host
sudo docker rm $host
done

for host in vxr-prod-procs01 vxr-prod-procs02 ; do
sudo docker stop $host
sudo docker rm $host
done

for host in vxr-prod-raicache01 vxr-prod-raicache02 ; do
sudo docker stop $host
sudo docker rm $host
done
