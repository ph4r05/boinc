
boinc init

OUT=`boinc resolve_filename out`
IN=`boinc resolve_filename in`
NUM=`cat ${IN}`

PERCENT_PER_ITER=$((100000 / NUM))

for i in `seq $NUM`; do
    PERCENT_COMPLETE=$((PERCENT_PER_ITER * i / 1000))
    boinc fraction_done_percent ${PERCENT_COMPLETE} 
    echo -e "I am ${PERCENT_COMPLETE}% complete." >> ${OUT}
    sleep 1;
done

boinc finish 0
