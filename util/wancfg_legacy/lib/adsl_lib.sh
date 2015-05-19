#!/bin/sh

function adsl_get_std_item () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Standard Item \
						"
	menu_options="'ADSL_T1_413' 		'T1.413' \
		      'ADSL_G_LITE' 		'G.lite (G992.2)' \
		      'ADSL_G_DMT'  		'G.dmt  (G992.1)' \
		      'ADSL_ALCATEL_1_4'	'Alcatel 1.4' \
		      'ADSL_ALCATEL'		'Alcatel' \
		      'ADSL_ADI'		'ADI' \
		      'ADSL_MULTIMODE' 		'Multimode (default)' \
		      'ADSL_T1_413_AUTO'	'T1.413 Auto' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL STANDARD ITEM " "$menu_options" 7 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_Standard=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_standard_item"
			adsl_get_standard_item
			;;
		*) 	return 1
			;;
	esac


}

function adsl_get_trellis () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Trellis \
						"
	menu_options="'ADSL_TRELLIS_ENABLE' 		'Enable (default)' \
		      'ADSL_TRELLIS_DISABLE'		'Disable' \
		      'ADSL_TRELLIS_LITE_ONLY_DISABLE'	'Lite only disable' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL TRELLIS " "$menu_options" 3 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_Trellis=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_trellis"
			adsl_get_trellis
			;;
		*) 	return 1
			;;
	esac

}

function adsl_get_codinggain () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Coding Gain \
						"
	menu_options="'ADSL_0DB_CODING_GAIN'	'0dB' \
		      'ADSL_1DB_CODING_GAIN'	'1dB' \
		      'ADSL_2DB_CODING_GAIN'	'2dB' \
		      'ADSL_3DB_CODING_GAIN'	'3dB' \
		      'ADSL_4DB_CODING_GAIN'	'4dB' \
		      'ADSL_5DB_CODING_GAIN'	'5dB' \
		      'ADSL_6DB_CODING_GAIN'	'6dB' \
		      'ADSL_7DB_CODING_GAIN'	'7dB' \
		      'ADSL_AUTO_CODING_GAIN'	'Auto Coding Gain (default)' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL CODING GAIN " "$menu_options" 9 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_CodingGain=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_codinggain"
			adsl_get_codinggain
			;;
		*) 	return 1
			;;
	esac

}


function adsl_get_rxbinadjust () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Rx BIN Adjust \
						"
	menu_options="'ADSL_RX_BIN_DISABLE'	'Disable (default)' \
		      'ADSL_RX_BIN_ENABLE'	'Enable' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL RX BIN ADJUST " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_RxBinAdjust=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_rxbinadjust"
			adsl_get_rxbinadjust
			;;
		*) 	return 1
			;;
	esac

}

function adsl_get_framingtype () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Framing Structure Type \
						"
	menu_options="'ADSL_FRAMING_TYPE_0'	'Type 0' \
		      'ADSL_FRAMING_TYPE_1'	'Type 1' \
		      'ADSL_FRAMING_TYPE_2'	'Type 2' \
		      'ADSL_FRAMING_TYPE_3'	'Type 3 (default)' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL FRAMING STRUCTURE TYPE " "$menu_options" 4 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_FramingStruct=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_framingtype"
			adsl_get_framingtype
			;;
		*) 	return 1
			;;
	esac

}

function adsl_get_expandedexchange () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Expanded Exchange \
						"
	menu_options="'ADSL_EXPANDED_EXCHANGE'	'Expanded (default)' \
		      'ADSL_SHORT_EXCHANGE'	'Short' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL Expanded Exchange " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_ExpandedExchange=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_expandedexchange"
			adsl_get_expandedexchange
			;;
		*) 	return 1
			;;
	esac

}

function adsl_get_clocktype () {

	local menu_options
	local rc
	local choice


	menu_instr="Please specify ADSL Clock Type \
						"
	menu_options="'ADSL_CLOCK_CRYSTAL'	'Crystal (default)' \
		      'ADSL_CLOCK_OSCILLATOR'	'Oscillator' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} ADSL CLOCK TYPE " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ADSL_ClockType=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "adsl_get_clocktype"
			adsl_get_clocktype
			;;
		*) 	return 1
			;;
	esac

}

