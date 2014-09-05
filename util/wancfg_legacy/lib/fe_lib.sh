# TE1 Select media connection for T1/E1 board.
function get_media_type () {

	local choice
	local rc

	menu_options="'T1' 'T1 connection' \
		      'E1' 'E1 connection' 2> $MENU_TMP"

	menu_instr="Please specify connection type. Refer to User Manual \
									"
	menu_name "CONNECTION TYPE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ $MEDIA != $choice ]; then
				MEDIA=$choice     #Choice is a command 
				if [ $MEDIA = T1 ]; then 
					LCODE="B8ZS"
					FRAME="ESF"
				else
					LCODE="HDB3"
					FRAME="NCRC4"
				fi
			fi		
			;;
		2)	device_setup_help "get_media_type" 
			get_media_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select line decoding for T1/E1 board.
function get_lcode_type () {

	local choice
	local rc

	if [ $MEDIA = T1 ]; then
		menu_options="'AMI'  'AMI line decoding' \
		      	      'B8ZS' 'B8ZS line decoding' 2> $MENU_TMP"
	else
		menu_options="'AMI'  'AMI line decoding' \
			      'HDB3' 'HDB3 line decoding' 2> $MENU_TMP"
	fi

	menu_instr="Please specify line decoding. Refer to User Manual \
									"
	menu_name "LINE DECODING" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LCODE=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_lcode_type" 
			get_lcode_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select frame type for T1/E1 board.
function get_frame_type () {

	local choice
	local rc

	if [ $MEDIA = T1 ]; then
		menu_options="'D4'   'D4 framing format' \
			      'ESF'  'ESF framing format' \
			      'UNFRAMED'  'Unframed format' 2> $MENU_TMP"
	else
		menu_options="'NCRC4' 'non-CRC4 framing format' \
			      'CRC4'  'CRC4 framing format' \
			      'UNFRAMED'  'Unframed format' 2> $MENU_TMP"
	fi

	menu_instr="Please specify frame mode. Refer to User Manual \
									"
	menu_name "FRAMING FORMAT" "$menu_options" 3 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	FRAME=$choice     #Choice is a command 
			if [ $FRAME = "UNFRAMED" ]; then
				if [ $PROTOCOL = WAN_BITSTRM ]; then
					init_bstrm_opt
				fi
			fi
			;;
		2)	device_setup_help "get_frame_type" 
			get_frame_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select clock mode for T1/E1 mode.
function get_te_clock () {

	local choice
	local rc

	menu_options="'NORMAL' 'Normal (default)' \
		      'MASTER' 'Master' 2> $MENU_TMP"

	menu_instr="Please specify T1/E1 clock mode. Refer to User Manual \
									"
	menu_name "T1/E1 CLOCK MODE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	TE_CLOCK=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_te_clock" 
			get_te_clock
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select LBO type for T1/E1 board (only T1 connection).
function get_lbo_type () {

	local choice
	local rc

	menu_options="'0dB'       'CSU: 0dB' 		\
		      '7.5dB'     'CSU: 7.5dB' 		\
		      '15dB'      'CSU: 15dB' 		\
		      '22.5dB'    'CSU: 22.5dB'		\
		      '0-110ft'   'DSX: 0-110ft'	\
		      '110-220ft' 'DSX: 110-220ft' 	\
		      '220-330ft' 'DSX: 220-330ft' 	\
		      '330-440ft' 'DSX: 330-440ft' 	\
		      '440-550ft' 'DSX: 440-550ft' 	\
		      '550-660ft' 'DSX: 550-660ft' 2> $MENU_TMP"

	menu_instr="Please specify line build out type. Refer to User Manual \
									"
	menu_name "LINE BUILD OUT" "$menu_options" 10 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LBO=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_lbo_type" 
			get_lbo_type
			;;
		*) 	return 1
			;;
	esac
}

function get_highimpedance_type () {

	local choice
	local rc

	menu_options="'NO'  'Disable High-Impedance mode (default)' \
		      'YES' 'Enable High-Impedance mode' 2> $MENU_TMP"

	menu_instr="Enable or Disable High-Impedance Mode \
						"
	menu_name "HIGH-IMPEDANCE MODE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	HIGHIMPEDANCE=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_highimpedance_type" 
			get_highimpedance_type
			;;
		*) 	return 1
			;;
	esac
}


function get_encoding () {

	local choice
	local rc
	local num=$1

	menu_options="'NO'   'RAW-Pure IP interface type' \
		      'YES'  'True interface encoding typy' 2> $MENU_TMP"

	menu_instr="Please specify the WANPIPE interface type setting \
									"

	menu_name "WANPIPE INTERFACE TYPE OPTION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	TRUE_ENCODING[$num]=$choice     #Choice is a command 
			;;
		2)	interface_setup_help "get_encoding" 
			get_encoding $num
			;;
		*) 	return 1
			;;
	esac
}

#------ TE3 ---------------------







# TE1 Select media connection for T1/E1 board.
function get_media_te3_type () {

	local choice
	local rc

	menu_options="'DS3' 'DS3/T3 connection' \
		      'E3'  'E3 connection' 2> $MENU_TMP"

	menu_instr="Please specify connection type. Refer to User Manual \
									"
	menu_name "CONNECTION TYPE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ $MEDIA != $choice ]; then
				MEDIA=$choice     #Choice is a command 
				if [ $MEDIA = DS3 ]; then 
					LCODE="B3ZS"
					FRAME="C-BIT"
				else
					LCODE="HDB3"
					FRAME="G.751"
				fi
			fi		
			;;
		2)	device_setup_help "get_media_te3_type" 
			get_media_te3_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select line decoding for T1/E1 board.
function get_lcode_te3_type () {

	local choice
	local rc

	if [ $MEDIA = DS3 ]; then
		menu_options="'B3ZS' 'B3ZS line decoding' \
		      	      'AMI'  'AMI line decoding' 2> $MENU_TMP"
	else
		menu_options="'HDB3' 'HDB3 line decoding' \
			      'AMI'  'AMI line decoding' 2> $MENU_TMP"
	fi

	menu_instr="Please specify line decoding. Refer to User Manual \
									"
	menu_name "LINE DECODING" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LCODE=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_lcode_te3_type" 
			get_lcode_te3_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select frame type for T1/E1 board.
function get_frame_te3_type () {

	local choice
	local rc

	if [ $MEDIA = DS3 ]; then
		menu_options="'C-BIT' 'C-BIT framing format' \
			      'M13'   'M13 framing format' 2> $MENU_TMP"
	else
		menu_options="'G.751'  'G.751 framing format' \
			      'G.832'  'G.832 framing format' 2> $MENU_TMP"
	fi

	menu_instr="Please specify frame mode. Refer to User Manual \
									"
	menu_name "FRAMING FORMAT" "$menu_options" 3 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	FRAME=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_frame_te3_type" 
			get_frame_te3_type
			;;
		*) 	return 1
			;;
	esac
}

# TE1 Select media connection for T1/E1 board.
function get_TE3_FCS () {

	local choice
	local rc

	menu_options="'16' 'CRC 16' \
		      '32' 'CRC 32' 2> $MENU_TMP"

	menu_instr="Please specify CRC type. Refer to User Manual \
									"
	menu_name "CRC/FCS TYPE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	TE3_FCS=$choice
			;;
		2)	device_setup_help "get_TE3_FCS" 
			get_TE3_FCS
			;;
		*) 	return 1
			;;
	esac
}

