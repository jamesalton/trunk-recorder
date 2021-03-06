#include "smartnet_parser.h"

using namespace std;
SmartnetParser::SmartnetParser() {
	lastaddress = 0;
	lastcmd = 0;
}
double SmartnetParser::getfreq(int cmd) {


/* Different Systems will have different band plans. Below is the one for WMATA which is a bit werid:*/
 /*       if (cmd < 0x12e) {
                freq = float((cmd) * 0.025 + 489.0875);
        } else if (cmd < 0x2b0) {
                freq = float((cmd-380) * 0.025 + 489.0875);
        } else {
                freq = 0;
        }
      cout << "CMD: 0x" <<  hex << cmd << " Freq: " << freq << " Multi: " << (cmd - 308) * 0.025 << " CMD: " << dec << cmd << endl; 
*/
	float freq;
	if (cmd < 0x1b8) {
		freq = float(cmd * 0.025 + 851.0125);
	} else if (cmd < 0x230) {
		freq = float(cmd * 0.025 + 851.0125 - 10.9875);
	} else {
		freq = 0;
	}

	return freq*1000000;
}



std::vector<TrunkMessage> SmartnetParser::parse_message(std::string s) {
	std::vector<TrunkMessage> messages;
	TrunkMessage message;


	message.message_type = UNKNOWN;
	message.encrypted = false;
	message.tdma = false;
	message.source = 0;
	message.sysid = 0;
	message.emergency = false;

	std::vector<std::string> x;
	boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);

	int full_address = atoi( x[0].c_str() );
	int status = full_address & 0x000F;
	long address = full_address & 0xFFF0;
	//int groupflag = atoi( x[1].c_str() );
	int command = atoi( x[2].c_str() );

	x.clear();
	vector<string>().swap(x);
	if ((address & 0xfc00) == 0x2800) {
		message.sysid = lastaddress;
		message.message_type = SYSID;
	} else if (command < 0x2d0) {
		message.talkgroup = address;
		message.freq = getfreq(command);
		if ( lastcmd == 0x308 || lastcmd == 0x321 ) { // Include digital
			// Channel Grant
			message.message_type = GRANT;
			message.source = lastaddress;
			// Check Status
			/* Status Message in TalkGroup ID
			 *   0 Normal Talkgroup
			 *   1 All Talkgroup
			 *   2 Emergency
			 *   3 Talkgroup patch to another
			 *   4 Emergency Patch
			 *   5 Emergency multi - group
			 *   6 Not assigned
			 *   7 Multi - select (initiated by dispatcher)
			 *   8 DES Encryption talkgroup
			 *   9 DES All Talkgroup
			 *  10 DES Emergency
			 *  11 DES Talkgroup patch
			 *  12 DES Emergency Patch
			 *  13 DES Emergency multi - group
			 *  14 Not assigned
			 *  15 Multi - select DES TG
			 */
			if(status == 2 && status == 4 && status == 5) {
				message.emergency = true;
			} else if ( status >= 8 ) { // Ignore DES Encryption
				message.message_type = UNKNOWN;
			}
		} else {
			// Call continuation
			message.message_type = UPDATE;
		}
	} else if (command == 0x03c0) {
		message.message_type = STATUS;
		//parse_status(command, address,groupflag);
	}

	lastaddress = full_address;
	lastcmd = command;
	messages.push_back(message);
	return messages;
}
