
#include <robosub/serial.h>
#include <robosub/util.h>

using namespace std;
using namespace robosub;

int main(){
	//cout<<"enter port (i.e. /dev/ttyACM0): ";
	//string port; //i.e. /dev/ttyACM0 or /dev/ttyUSB1
	//cin>>port;   //ls /dev | grep tty[AU]
	
	//Serial serial(port.c_str());
	
	string port = Util::execCLI("ls /dev | grep tty[AU]");
	cout<<"using serial port /dev/"<<port<<endl;
	Serial serial = Serial("/dev/" + port.substr(0,port.length()-1));
  
	int problemcounter = 0;
  
	while(true){
		char serdata[100];
		int serlen;
		
		serlen = serial.readToChar(serdata, 99, '\0');
		
		if(serlen>0){
			serdata[serlen] = '\0';
			printf("%s\n",serdata);
			if(serdata[0]!='h'){
				problemcounter++;
				if(problemcounter>2){
					break;
				}
			}
		}else if(serlen==-1){
			cout<<"not enough buffer"<<endl;
		}else if(serlen==-2){
			//cout<<"not found"<<endl;
		}
	}
	
	return 0;
}
