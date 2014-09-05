#!/bin/bash

#Zaptel strings to be replaced during patch
ZAPTEL_C_SEARCH_STR="chan->writen\[chan->inwritebuf\] = amnt;"
ZAPTEL_C_PATCH="if ((chan->flags \& ZT_FLAG_HDLC) \&\& chan->span->ioctl != NULL){\n\
                                if (chan->span->ioctl(chan, ZT_DCHAN_TX_V2, amnt)==0){\n\
                                        return amnt;\n\
                                }\n\
                        }\n\
                        chan->writen[chan->inwritebuf] = amnt;"

# D channel patching for zaptel version 1.2.17.1/1.4.2.1 or above
ZAPTEL_C_SEARCH_STR_NEW="chan->writen\[res\] = amnt;"
ZAPTEL_C_PATCH_NEW="if ((chan->flags \& ZT_FLAG_HDLC) \&\& chan->span->ioctl != NULL){\n\
                                if (chan->span->ioctl(chan, ZT_DCHAN_TX_V2, amnt)==0){\n\
                                        return amnt;\n\
                                }\n\
                        }\n\
                        chan->writen[res] = amnt;"

#NOTE1: the 'sed' command does NOT work on more than one line, so the
#closing '*/' will be pushed AFTER the inserted strings.

ZAPTEL_H_SEARCH_STR=" \*  80-85 are reserved for dynamic span stuff"
ZAPTEL_H_PATCH=\
" \*  80-85 are reserved for dynamic span stuff\n\
 \*\/\n\
#define ZT_DCHAN_TX     _IOR (ZT_CODE, 60, int)\n\
#define ZT_DCHAN_TX_V1  ZT_DCHAN_TX\n\
#define ZT_DCHAN_TX_V2  ZT_DCHAN_TX\n\/\*"


ZAPTEL_C_FILE=zaptel.c

tdmv_apply_zaptel_dchan_patch ()
{
        lhome=`pwd`

        cd $ZAPTEL_DIR
	if [ -e $ZAPTEL_DIR/zaptel-base.c ]; then
		ZAPTEL_C_FILE=zaptel-base.c
	fi

        eval "grep \"ZT_DCHAN_TX_V2\" $ZAPTEL_C_FILE > /dev/null 2> /dev/null"
        if [ $? -eq 0 ]; then
                echo "   Zaptel DCHAN patch V.02 already installed"
                cd $lhome
                return 1
        fi

        # patch zaptel.h
        eval "search_and_replace zaptel.h zaptel.h \"$ZAPTEL_H_SEARCH_STR\" \"$ZAPTEL_H_PATCH\""
        if [ $? -ne 0 ]; then
                echo "search_and_replace(zaptel.h) failed"
                cd $lhome
                return 2
        else
               echo "search_and_replace(zaptel.h) succeeded"
        fi


	# patch $ZAPTEL_C_FILE
        eval "grep \"$ZAPTEL_C_SEARCH_STR\"  $ZAPTEL_C_FILE > /dev/null 2> /dev/null"
	if [ $? -eq 0 ]; then
		eval "search_and_replace $ZAPTEL_C_FILE $ZAPTEL_C_FILE \"$ZAPTEL_C_SEARCH_STR\" \"$ZAPTEL_C_PATCH\""
	else
		eval "search_and_replace $ZAPTEL_C_FILE $ZAPTEL_C_FILE \"$ZAPTEL_C_SEARCH_STR_NEW\" \"$ZAPTEL_C_PATCH_NEW\""
	fi

        if [ $? -ne 0 ]; then
                echo "search_and_replace($ZAPTEL_C_FILE) failed"
                cd $lhome
                return 2
        else
                echo "search_and_replace($ZAPTEL_C_FILE) succeeded"
        fi
        cd $lhome
	return 0
}


search_and_replace()
{
        local input_file_name=$1
        local output_file_name=$2
        local search_str="$3"
        local replace_str="$4"
        local tmp_file=output.tmp

        #echo "input_file_name:$input_file_name"
        #echo "output_file_name:$output_file_name"
        #echo "search_str:$search_str"
        #echo "replace_str:$replace_str"

        eval "grep '$search_str' $input_file_name > /dev/null 2> /dev/null"

        if [ $? -ne 0 ]; then
                #echo "Did NOT find the seached str:$search_str"
                return 1
        else
                #echo "Found the seached str"
                eval "sed 's/'\"$search_str\"'/'\"$replace_str\"'/' $input_file_name > $tmp_file"
                mv $tmp_file $output_file_name
        fi

        return $?
}

ZAPTEL_DIR=$1;
if [ "$ZAPTEL_DIR" == "" ]; then
	echo "Zaptel source directory not specified, using source in /usr/src/zaptel"
	echo " "
	ZAPTEL_DIR=/usr/src/zaptel
fi

cd $ZAPTEL_DIR

if [ -d kernel ]; then
	ZAPTEL_DIR=$ZAPTEL_DIR/kernel
fi

if [ -e $ZAPTEL_DIR/zaptel-base.c ]; then
        ZAPTEL_C_FILE=$ZAPTEL_DIR/zaptel-base.c
fi


eval "grep hdlc_hard_xmit $ZAPTEL_C_FILE > /dev/null 2> /dev/null"
if [ $? -eq 0 ] ; then
	if [ ! -e /etc/wanpipe ]; then
		eval "\mkdir -p /etc/wanpipe 2> /dev/null"
	fi
	if [ -e /etc/wanpipe ]; then
		touch /etc/wanpipe/.zaphdlc
	fi
	echo "Zaptel HW HDLC support detected"
	echo
	exit 10
else
	if [ -e  /etc/wanpipe/.zaphdlc ]; then
		eval "rm -f  /etc/wanpipe/.zaphdlc 2> /dev/null"
	fi
fi

echo "Applying Sangoma DCHAN patch to $ZAPTEL_DIR"
echo " "
tdmv_apply_zaptel_dchan_patch
rc=$?

if [ $rc == 0 ]; then
 	echo "Please recompile and re-install ZAPTEL, zaptel source changed!"
fi

exit $rc
 
