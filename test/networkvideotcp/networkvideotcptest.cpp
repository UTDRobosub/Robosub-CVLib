
#include <thread>
#include <opencv2/opencv.hpp>
#include <robosub/robosub.h>
#include <robosub/networktcp.h>
#include <signal.h>
#include <opencv2/ximgproc.hpp>

using namespace std;
using namespace robosub;

bool running = false;

int displayFrames(int mode, int port, const String &addr, bool showDisplay, const string &camera, Size &frameSize);

void catchSignal(int signal) {
    running = false;
}

const int MODE_RECEIVE = 0;
const int MODE_SEND = 1;

const int VERIFICATION_CODE = 1234567890;


bool mainloop = true;

void drawFrame(int rows, int cols, char* framedata, float framesPerSecond, float bitsPerSecond){
	int framedatalen = rows*cols*3;
	
	Mat bestframedraw(rows, cols, CV_8UC3, framedata);
	
	Drawing::text(bestframedraw,
		String(Util::toStringWithPrecision(framesPerSecond)) + String(" fps"),
		Point(16,48), Scalar(255,255,255), Drawing::Anchor::BOTTOM_LEFT, 0.5
	);
	Drawing::text(bestframedraw,
		String(Util::toStringWithPrecision((bitsPerSecond)/1024.0f/1024.0f) + String(" Mbps")),
		Point(16,16), Scalar(255, 255, 255), Drawing::Anchor::BOTTOM_LEFT, 0.5
	);
	
	imshow("Latest Frame", bestframedraw);
}

int main(int argc, char **argv) {

    const String keys =
            "{help ?         |            | print this message     }"
            "{@mode          |            | 'send' or 'receive'    }"
            "{p port         |8500        | port to send/listen to }"
            "{d no-display   |false       | disable visualization (send only, faster) }"
            "{h host         |127.0.0.1   | address to send to/receive from }"
            "{vc cols        |1280        | image buffer columns (send only)  }"
            "{vr rows        |720         | image buffer rows (send only)  }"
            "{c cam camera   |/dev/video0 | camera id (send only) }";

//	cout << cv::getBuildInformation() << endl;


	CommandLineParser parser(argc, argv, keys);
	parser.about("Network Video Transfer Test");
	
	if (parser.has("help"))
	{
		parser.printMessage();
		return 0;
	}
	
	if (!parser.has("@mode")) {
			cout << "Mode is required." << endl << endl;
			parser.printMessage();
			return 0;
	}
	if (!parser.check())
	{
		parser.printErrors();
		return 0;
	}
	
	int mode = parser.get<String>("@mode")[0] == 's' ? MODE_SEND : MODE_RECEIVE;
	int port = parser.get<int>("port");
	String addr = parser.get<String>("host");
	bool showDisplay = !parser.get<bool>("d");
	const string camera = parser.get<string>("camera");
	Size frameSize = Size(parser.get<int>("cols"), parser.get<int>("rows"));
	
	//catch signal
	signal(SIGPIPE, catchSignal);
	
	Size screenRes;
	Camera *cam;
	if(mode == MODE_SEND){
		cam = new Camera(camera);
		if (!cam->isOpen()){
			cout<<"Camera failed to open."<<endl;
			return -1;
		}
		frameSize = cam->setFrameSize(frameSize);
		cout << frameSize << endl;
	} else {
		screenRes = Util::getDesktopResolution();
	}

	//these can be overridden by the receiver
	int cols = frameSize.width;
	int rows = frameSize.height;
	
	//load calibration data - run AFTER resolution set
	Camera::CalibrationData calibrationData = *Camera::loadCalibrationDataFromXML("../config/fisheye180_cameracalib_fisheye.xml", frameSize);
	
	const int datalen = rows*cols*3 + 16;
	//4 bytes for cols
	//4 bytes for rows
	//framelen bytes for image data

    NetworkTcpServer server;

	while(mainloop) {

	    running = true;

        if (mode == MODE_SEND) {

            char *senddata = (char *) malloc(datalen);
            Mat frame1;

            cout << "Unbinding from port" << endl;
            server.unbindFromPort();

            cout << "Binding to port." << endl;
            server.bindToPort(port);

            cout << "Accepting client." << endl;
            server.acceptClient();

            cout << "Connected." << endl;

            float uploadBitsPerSecond = 0;

            while (running) {

                cam->retrieveFrameBGR(frame1);

                *(int *) (senddata + 0) = VERIFICATION_CODE;
                *(int *) (senddata + 4) = cols;
                *(int *) (senddata + 8) = rows;
                *(int *) (senddata + 12) = 0;
                memcpy(senddata + 16, frame1.data, rows * cols * 3);

                //test: break the frame into 100 segments and send one every 100 us (10 ms per frame)
                int segmentsize = datalen / 100;
                int numsegments = datalen / segmentsize + 1;
                for (int i = 0; i < numsegments; i++) {
                    int ecode = server.sendBuffer(senddata + segmentsize * i, min(segmentsize, datalen - segmentsize * i));
                    if (ecode != 0) {
                        cout << "Send error: " << ecode << " " << strerror(ecode) << endl;
                        break;
                    }

                    robosub::Time::waitMicros(100);
                }

                uploadBitsPerSecond = ((float) cam->getFrameRate()) * ((float) ((rows * cols * 3 + 16) * 8));

                if (showDisplay) {
                    Drawing::text(frame1,
                                  String(Util::toStringWithPrecision(cam->getFrameRate())) + String(" fps"),
                                  Point(16, 48), Scalar(255, 255, 255), Drawing::Anchor::BOTTOM_LEFT, 0.5
                    );
                    Drawing::text(frame1,
                                  String(Util::toStringWithPrecision(uploadBitsPerSecond / 1024.0 / 1024.0)) +
                                  String(" Mbps"),
                                  Point(16, 16), Scalar(255, 255, 255), Drawing::Anchor::BOTTOM_LEFT, 0.5
                    );

                    imshow("Sending Frame", frame1);
                }

                waitKey(1);
            }
        } else {

            FPS fps = FPS();

            float framesPerSecond;
            float bitsPerSecond;

            NetworkTcpClient client;

            cout << "Connecting to server." << endl;
            int err = client.connectToServer((char *) addr.c_str(), port);
            if (err) {
                cout << "Connection Error " << err << ": " << strerror(err) << endl;
            } else {
                cout << "Connected." << endl;

                char *framedata = nullptr;

                char *headerdata = (char *) malloc(16);

                int waitingOnRestOfFrame = 0;
                int framedatalen = 0;

                while (running) {

                    if (waitingOnRestOfFrame == 0) {
                        int headerlen = client.receiveBuffer(headerdata, 16);

                        if (headerlen == 16) {
                            int ver1 = *(int *) (headerdata + 0);
                            cols = *(int *) (headerdata + 4);
                            rows = *(int *) (headerdata + 8);
                            int none = *(int *) (headerdata + 12);

                            //cout<<cols<<" "<<rows<<endl;

                            if (ver1 == VERIFICATION_CODE) {

                                fps.frame();

                                framesPerSecond = (float) fps.fps();
                                bitsPerSecond = (float) (framesPerSecond * (framedatalen + 16) * 8);

                                framedatalen = rows * cols * 3;

                                if (framedata != nullptr) {
                                    free(framedata);
                                }

                                framedata = (char *) malloc(framedatalen);

                                int recvdatalen = client.receiveBuffer(framedata, framedatalen);

                                if (recvdatalen >= 0) {
                                    waitingOnRestOfFrame = framedatalen - recvdatalen;
                                    //cout<<"waiting on "<<waitingOnRestOfFrame<<endl;

                                    if (waitingOnRestOfFrame == 0) {
                                        drawFrame(rows, cols, framedata, framesPerSecond, bitsPerSecond);
                                    }
                                }
                            }
                        }
                    } else {
                        int recvdatalen = client.receiveBuffer(framedata + (framedatalen - waitingOnRestOfFrame),
                                                               waitingOnRestOfFrame);

                        if (recvdatalen >= 0) {
                            waitingOnRestOfFrame = waitingOnRestOfFrame - recvdatalen;

                            //cout<<"waiting on "<<waitingOnRestOfFrame<<endl;

                            if (waitingOnRestOfFrame == 0) {
                                drawFrame(rows, cols, framedata, framesPerSecond, bitsPerSecond);
                            }
                        } else {
                            cout << errno << endl;
                        }
                    }

                    waitKey(1);
                }
            }
        }
    }

	cout << "Safely, nicely exited" << endl;
	
	return 0;
}
