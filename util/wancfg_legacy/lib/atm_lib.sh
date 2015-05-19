function atm_interface_init()
{
	local num="$1"

	atm_oam_loopback[$num]=${atm_oam_loopback[$num]:-NO}
	atm_oam_loopback_int[$num]=${atm_oam_loopback_int[$num]:-5}
	
	atm_oam_cc_check[$num]=${atm_oam_cc_check[$num]:-NO}
	atm_oam_cc_check_int[$num]=${atm_oam_cc_check_int[$num]:-2}
	
	atm_arp[$num]=${atm_arp[$num]:-NO}
	atm_arp_int[$num]=${atm_arp_int[$num]:-15}
}

function atm_interface_setup()
{
	local rc
	local num="$1"
	local choice
	local menu_options

	atm_interface_init $num

	menu_options="'atm_oam_loopback'     'OAM Loopback -----------> ${atm_oam_loopback[$num]}' \
		      'atm_oam_loopback_int' 'OAM Loopback Interval --> ${atm_oam_loopback_int[$num]}' \
		      'atm_oam_cc_check'     'OAM Continuity Check ---> ${atm_oam_cc_check[$num]}' \
		      'atm_oam_cc_check_int' 'OAM Continuity Interval-> ${atm_oam_cc_check_int[$num]}' \
	              'again'                ' ' \
		      'atm_arp'		     'ATM ARP ----------------> ${atm_arp[$num]}' \
		      'atm_arp_int'          'ATMARP_INT -------------> ${atm_arp_int[$num]}' 2> $MENU_TMP"

	
	menu_instr="Please specify the ${IF_NAME[$num]}: ATM protocol parameters below \
				"

	menu_name "ATM ${IF_NAME[$num]} PROTOCOL SETUP" "$menu_options" 12 "$menu_instr" "$BACK" 
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 

			atm_oam_loopback)
				warning "atm_oam_loopback"
					if [ $? -eq 0 ]; then
						atm_oam_loopback[$num]=YES
					else
						atm_oam_loopback[$num]=NO
					fi
				;;

			atm_oam_loopback_int)
				get_string "Please specify ATM OAM Loopback interval" "${atm_oam_loopback_int[$num]}"	
				atm_oam_loopback_int[$num]=$($GET_RC)
				;;

			atm_oam_cc_check)
				warning "atm_oam_cc_check"
					if [ $? -eq 0 ]; then
						atm_oam_cc_check[$num]=YES
					else
						atm_oam_cc_check[$num]=NO
					fi
				;;

			atm_oam_cc_check_int)
				get_string "Please specify ATM Continuity Interval" "${atm_oam_cc_check_int[$num]}"	
				atm_oam_cc_check_int[$num]=$($GET_RC)
				;;
	
			atm_arp)
				warning "atm_arp"
					if [ $? -eq 0 ]; then
						atm_arp[$num]=YES
					else
						atm_arp[$num]=NO
					fi
				;;
			atm_arp_int)
				get_string "Please specify ATM ARP Interval" "${atm_arp_int[$num]}"	
				atm_arp_int[$num]=$($GET_RC)
				;;
			
		*)
				$choice    #Choice is a command 
				;;	
			esac
			return 0	
			;;
			
		2) 	choice=${choice%\"*\"}
			choice=${choice// /}
			interface_setup_help $choice	
			return 0
			;;
		*)
			return 1
			;;
	esac
}
