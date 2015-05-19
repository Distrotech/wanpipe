

CFILES=`ls *.c`
for cfile in $CFILES
do
	file=${cfile%%.c*}
	rm -f $file".o" 
done

