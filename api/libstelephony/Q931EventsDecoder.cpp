
#if defined (__WINDOWS__)
# include <windows.h>
# define NULL_CHAR NULL
#elif defined (__LINUX__)
# define NULL_CHAR 0
#endif

#include "Q931EventsDecoder.h"


char ia5[16][8]={{NULL_CHAR,NULL_CHAR,' ','0','@','P','`','p'},
				{NULL_CHAR,NULL_CHAR,'!','1','A','Q','a','q'},
				{NULL_CHAR,NULL_CHAR,'"','2','B','R','b','r'},
				{NULL_CHAR,NULL_CHAR,'#','3','C','S','c','s'},
				{NULL_CHAR,NULL_CHAR,'$','4','D','T','d','t'},
				{NULL_CHAR,NULL_CHAR,'%','5','E','U','e','u'},
				{NULL_CHAR,NULL_CHAR,'&','6','F','V','f','v'},
				{NULL_CHAR,NULL_CHAR,'\'','7','G','W','g','w'},
				{NULL_CHAR,NULL_CHAR,'(','8','H','X','h','x'},
				{NULL_CHAR,NULL_CHAR,')','9','I','Y','i','y'},
				{NULL_CHAR,NULL_CHAR,'*',':','J','Z','j','z'},
				{NULL_CHAR,NULL_CHAR,'+',';','K','[','k','{'},
				{NULL_CHAR,NULL_CHAR,',','<','L','\\','l','|'},
				{NULL_CHAR,NULL_CHAR,'-','=','M',']','m','}'},
				{NULL_CHAR,NULL_CHAR,'.','>','N','^','n','~'},
				{NULL_CHAR,NULL_CHAR,'/','?','O','_','o',NULL_CHAR}};

Q931EventsDecoder::Q931EventsDecoder(void)
{
	DBG_Q931("%s(): this ptr: 0x%p\n", __FUNCTION__, this);

	memset(&q931_event_decoder_callback_functions, 0x00, sizeof(q931_event_decoder_callback_functions_t));
	callback_obj = NULL;
	m_IsEnabled = FALSE;
}

Q931EventsDecoder::~Q931EventsDecoder(void)
{
	DBG_Q931("%s(): this ptr: 0x%p\n", __FUNCTION__, this);
}

void Q931EventsDecoder::SetCallbackFunctions(IN q931_event_decoder_callback_functions_t *cf)
{
	DBG_Q931("%s(): cf->OnQ931Event: 0x%p\n", __FUNCTION__, cf->OnQ931Event);

	memcpy(&q931_event_decoder_callback_functions, cf, sizeof(q931_event_decoder_callback_functions_t));
}

void Q931EventsDecoder::SetCallbackObject(IN void *cbo)
{
	callback_obj = cbo;
}

void Q931EventsDecoder::GetCallbackFunctions(OUT q931_event_decoder_callback_functions_t *cf)
{
	memcpy(cf, &q931_event_decoder_callback_functions, sizeof(q931_event_decoder_callback_functions_t));
}

stelephony_status_t Q931EventsDecoder::EventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData)
{
	switch(Event)
	{
	case STEL_EVENT_Q931:
		switch(ControlCode)
		{
		case STEL_CTRL_CODE_ENABLE:
			m_IsEnabled = TRUE;
			break;
		case STEL_CTRL_CODE_DISABLE:
			m_IsEnabled = FALSE;
			break;
		}
		break;
	default:
		return STEL_STATUS_INVALID_EVENT_ERROR;
	}

	return STEL_STATUS_SUCCESS;
}

int Q931EventsDecoder::Input(void *vdata, int dataLength)
{
	//Variables
	stelephony_q931_event q931_event;	//structure for q931 event
	unsigned char *data=(unsigned char *)vdata;  //recast it to a char so that it is easy to manipulate
	int i,c,j,len;

	//initalize the q931_event structure
	q931_event.called_num_digits[0]=NULL_CHAR;
	q931_event.called_num_digits_count=0;
	q931_event.calling_num_digits[0]=NULL_CHAR;
	q931_event.calling_num_digits_count=0;
	q931_event.calling_num_presentation=0;
	q931_event.calling_num_screening_ind=0;
	q931_event.cause_code=0;
	q931_event.chan=0;
	q931_event.data[0]=NULL_CHAR;
	q931_event.dataLength=0;
	q931_event.len_callRef=0;
	q931_event.msg_type[0]=NULL_CHAR;
	q931_event.rdnis_digits_count=0;
	q931_event.rdnis_string[0]=NULL_CHAR;

	//debug print showing we entered this function
	//DBG_Q931("%s()\n", __FUNCTION__);

	if(m_IsEnabled == FALSE){
		//Disabled - not an error.
		return STEL_STATUS_OBJECT_DISABLED_ERROR;
	}

	if(!q931_event_decoder_callback_functions.OnQ931Event){
		//Enabled - but no callback function - error!
		return STEL_STATUS_REQUEST_INVALID_FOR_CURRENT_OBJECT_STATE_ERROR;
	}

	//Check if data is long enough and if this is a Q931 frame
	if ((dataLength < 5) || ((int)data[4] != 8) ){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}

	//Time stamp this frame
	GetLocalTime(&q931_event.tv);

	q931_event.dataLength=dataLength;
	//memcpy(&q931_event.data,&data,dataLength);
	for (i=0;i<dataLength;i++){
		q931_event.data[i]=data[i];
	}

	//Extract the length of the call reference field
	q931_event.len_callRef=(int)(data[5] & 0x0F);
	c=6;
	for ( i=0;i<(2*q931_event.len_callRef);i++){
		switch ((int)(data[c] & 0xF0)) {
			case 0:
				q931_event.callRef[i]='0';
				break;
			case 16:
				q931_event.callRef[i]='1';
				break;
			case 32:
				q931_event.callRef[i]='2';
				break;
			case 48:
				q931_event.callRef[i]='3';
				break;
			case 64:
				q931_event.callRef[i]='4';
				break;
			case 80:
				q931_event.callRef[i]='5';
				break;
			case 96:
				q931_event.callRef[i]='6';
				break;
			case 112:
				q931_event.callRef[i]='7';
				break;
			case 128:
				q931_event.callRef[i]='8';
				break;
			case 144:
				q931_event.callRef[i]='9';
				break;
			case 160:
				q931_event.callRef[i]='A';
				break;
			case 176:
				q931_event.callRef[i]='B';
				break;
			case 192:
				q931_event.callRef[i]='C';
				break;
			case 208:
				q931_event.callRef[i]='D';
				break;
			case 224:
				q931_event.callRef[i]='E';
				break;
			case 240:
				q931_event.callRef[i]='F';
				break;
			default:
				q931_event.callRef[i]='?';
				break;
		}
		switch ((int)(data[c]& 0x0F)) {
			case 0:
				q931_event.callRef[i+1]='0';
				break;
			case 1:
				q931_event.callRef[i+1]='1';
				break;
			case 2:
				q931_event.callRef[i+1]='2';
				break;
			case 3:
				q931_event.callRef[i+1]='3';
				break;
			case 4:
				q931_event.callRef[i+1]='4';
				break;
			case 5:
				q931_event.callRef[i+1]='5';
				break;
			case 6:
				q931_event.callRef[i+1]='6';
				break;
			case 7:
				q931_event.callRef[i+1]='7';
				break;
			case 8:
				q931_event.callRef[i+1]='8';
				break;
			case 9:
				q931_event.callRef[i+1]='9';
				break;
			case 10:
				q931_event.callRef[i+1]='A';
				break;
			case 11:
				q931_event.callRef[i+1]='B';
				break;
			case 12:
				q931_event.callRef[i+1]='C';
				break;
			case 13:
				q931_event.callRef[i+1]='D';
				break;
			case 14:
				q931_event.callRef[i+1]='E';
				break;
			case 15:
				q931_event.callRef[i+1]='F';
				break;
		}
		i=i+1;
		c=c+1;
	}
	q931_event.callRef[i]=NULL_CHAR;
	
	//Extract the Caller Reference number and place it into the struct ***it can be up to 119bits long***

	//Find the Message type
	switch ((int)(data[6+q931_event.len_callRef] & 0xFF)){
		case 1:
			strcpy(q931_event.msg_type, "ALERTING");
			break;
		case 2:
			strcpy(q931_event.msg_type, "CALL_PROCEEDING");
			break;
		case 3:
			strcpy(q931_event.msg_type, "PROGRESS");
			break;
		case 5:
			strcpy(q931_event.msg_type, "SETUP");
			break;
		case 7:
			strcpy(q931_event.msg_type, "CONNECT");
			break;
		case 13:
			strcpy(q931_event.msg_type, "SETUP_ACK");
			break;
		case 15:
			strcpy(q931_event.msg_type, "CONNECT_ACK");
			break;
		case 32:
			strcpy(q931_event.msg_type, "USER_INFO");
			break;
		case 33:
			strcpy(q931_event.msg_type, "SUSPEND_REJ");
			break;
		case 34:
			strcpy(q931_event.msg_type, "RESUME_REJ");
			break;
		case 37:
			strcpy(q931_event.msg_type, "SUSPEND");
			break;
		case 38:
			strcpy(q931_event.msg_type, "RESUME");
			break;
		case 45:
			strcpy(q931_event.msg_type, "SUSPEND_ACK");
			break;
		case 46:
			strcpy(q931_event.msg_type, "RESUME_ACK");
			break;
		case 69:
			strcpy(q931_event.msg_type, "DISCONNECT");
			break;
		case 70:
			strcpy(q931_event.msg_type, "RESTART");
			break;
		case 77:
			strcpy(q931_event.msg_type, "RELEASE");
			break;
		case 78:
			strcpy(q931_event.msg_type, "RELEASE_ACK");
			break;
		case 90:
			strcpy(q931_event.msg_type, "RELEASE_COMPL");
			break;
		case 96:
			strcpy(q931_event.msg_type, "SEGMENT");
			break;
		case 110:
			strcpy(q931_event.msg_type, "NOTIFY");
			break;
		case 117:
			strcpy(q931_event.msg_type, "STATUS_ENQUIRY");
			break;
		case 121:
			strcpy(q931_event.msg_type, "CONGEST_CNTRL");
			break;
		case 123:
			strcpy(q931_event.msg_type, "INFORMATION");
			break;
		case 125:
			strcpy(q931_event.msg_type, "STATUS");
			break;
		default:
			strcpy(q931_event.msg_type, "UNKNOWN");
			break;
	}

	//go through rest of data and look for important info
	for(i=7+q931_event.len_callRef;i < dataLength;i++){
		switch ((int)(data[i] & 0xFF)){
			case 0:
				//Segmentented Message
				//don't care about it
				break;
			case 4:
				//bearer capability
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 8:
				//cause code info
				c=i; //c is now the place holder for the rest of the information element decode
				c++;
				c++; //This octet contains the length, but we don't care about the length 
				c++;//next octet contains information about coding standard and location but we are not interested in that
				q931_event.cause_code=(int)(data[c] & 0x7F);
				i=c-1;
				break;
			case 16:
				//call identity 
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 20:
				//call state
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 24:
				//channel ID info
				c=i; //c is now the place holder for the rest of the information element decode
				c++;
				len=(int)(data[c] & 0xFF);  //grab the total length of this element
				c=c+3; //next two octets contain information we don't care about (increase current by 3)
				if (((int)(data[c] & 0x10)) == 0){
					//The channel number is stored in the last 4 bits of this octet
					q931_event.chan=(int)(data[c] & 0x0F);
				}else if(((int)(data[c] & 0x10)) == 1){
					//The channel number is stored as a map in the rest of the octets
				}else{
					q931_event.chan=-1;
				}
				i=c-1;
				break;
			case 30:
				//progress indicator
				//don't care about it
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 32:
				//network specific facilities
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 39:
				//notification indicator
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 40:
				//display
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 41:
				//date/time
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 44:
				//keypad facility
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 52:
				//signal
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 64:
				//information rate
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 66:
				//end to end transmit delay
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 67:
				//Transit Delay Selection and indication
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 68:
				//Packet layer binary parameters
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 69:
				//Packet Layer window size
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 70:
				//packet size
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 71:
				//closed user group
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 74:
				//reverse charging indication
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 108:
				//calling party number
				c=i; //c is now the place holder for the rest of the information element decode
				c++;
				len=(int)(data[c] & 0xFF);  //grab the total length of this element
				q931_event.calling_num_digits_count=len-2;
				c=c+2;	//next octet contains type of number and numbering plan...aka we don't care
				q931_event.calling_num_screening_ind= (int)(data[c] & 0x60);
				q931_event.calling_num_presentation= (int)(data[c] & 0x03);
				c++;
				for(j=0;j <len-2;j++){
					//convert the calling number from field from IA5 encoding to ASCII encoding
					q931_event.calling_num_digits[j]=ia5[((int)(data[c] & 0x0F))][(((int)(data[c] & 0xF0))/16)];
					c++;
				}
				q931_event.calling_num_digits[j]=NULL_CHAR;
				i=c-1;
				break;
			case 109:
				//calling party subaddress
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 112:
				//called party number
				c=i; //c is now the place holder for the rest of the information element decode
				c++;
				len=(int)(data[c] & 0xFF);  //grab the total length of this element
				q931_event.called_num_digits_count=len-2;
				c++; //next octet contains the type of number and numbering plan...aka we don't care
				c++;
				for(j=0;j <len-2;j++){
					//convert the calling number from field from IA5 encoding to ASCII encoding
					q931_event.called_num_digits[j]=ia5[((int)(data[c] & 0x0F))][(((int)(data[c] & 0xF0))/16)];
					c++;
				}
				q931_event.called_num_digits[j]=NULL_CHAR;
				i=c-1;
				break;
			case 113:
				//called party sub-address
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 116:
				////rdnis info
				c=i; //c is now the place holder for the rest of the information element decode
				c++;
				len=(int)(data[c] & 0xFF);  //grab the total length of this element
				c++;
				if ((int)(data[c] & 0x80) == 64){
					c=c+2; // skip the next two elemnts to get to the original number
					for(j=0;j <len-2;j++){
					//convert the calling number from field from IA5 encoding to ASCII encoding
					q931_event.rdnis_string[j]=ia5[((int)(data[c] & 0x0F))][(((int)(data[c] & 0xF0))/16)];
					c++;
					}
					q931_event.rdnis_string[j]=NULL_CHAR;
					q931_event.rdnis_digits_count=j;
				}else{
					c=c+3; //skip the next three elements to get the original number
					for(j=0;j <len-2;j++){
					//convert the calling number from field from IA5 encoding to ASCII encoding
					q931_event.rdnis_string[j]=ia5[((int)(data[c] & 0x0F))][(((int)(data[c] & 0xF0))/16)];
					c++;
					}
					q931_event.rdnis_string[j]=NULL_CHAR;
					q931_event.rdnis_digits_count=j;
				}
				i=c-1;
				break;
			case 120:
				//transit network selection
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 121:
				//restart indicator
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 124:
				//low layer compatibility
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 125:
				//high layer compatability
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 126:
				//user-user
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			case 127:
				//escape for extension
				//don't care about it
				i++;
				len=(int)(data[i] & 0xFF);
				i=i+len;
				break;
			default:
				break;
		}
	}

	q931_event_decoder_callback_functions.OnQ931Event(this->callback_obj, &q931_event);

	return STEL_STATUS_SUCCESS;
}

