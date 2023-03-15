
s=7501
home=/home/chris
rvsched=${home}/rai/RaiCore/RH7_x86_64/bin/rvsched
log=${home}/raims_test/log
sched=${home}/raims_test/data/sched

for host in vxr-prod-oms01 vxr-prod-oms02 vxr-prod-oms03 vxr-prod-oms04 vxr-prod-tsm01 vxr-prod-tsm02 vxr-prod-qs01 vxr-prod-qs02 vxr-prod-lm01 ; do
sudo docker exec -d ${host} ${rvsched} -input ${sched}_${s}.out -service 7500 -rate -log ${log}/sched_${host}.log
s=$(($s + 1))
done

