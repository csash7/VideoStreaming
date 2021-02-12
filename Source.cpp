#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 5002

SOCKET ConnectSocket = INVALID_SOCKET;
int iResult;
const int size_message_length_ = 16;

int connectServer(std::string ip_addr) {
    WSADATA wsaData;

    struct sockaddr_in clientService;

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        return 0;
    }

    //----------------------
    // Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(DEFAULT_PORT);
    inet_pton(AF_INET, ip_addr.c_str(), &clientService.sin_addr);

    //----------------------
    // Connect to server.
    iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 0;
    }

    return 1;
}


int sendMessage(std::string message) {
    int length = message.length();
    std::string length_str = std::to_string(length);
    std::string message_length =
        std::string(size_message_length_ - length_str.length(), '0') + length_str;
    iResult = send(ConnectSocket, message_length.c_str(), size_message_length_, 0);
    if (iResult != SOCKET_ERROR) {
        // Send message
        iResult = send(ConnectSocket, message.c_str(), length, 0);
        if (iResult == SOCKET_ERROR) {
            wprintf(L"send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 0;
        }
    }
    return 1;
}

std::string receiveClient() {
    // Receive length of the message
    char message_length[size_message_length_] = { 0 };
    int n = recv(ConnectSocket, message_length, size_message_length_, 0);
    std::string message_length_string(message_length);
    int length = std::stoi(message_length_string);
    // if (length == 0) return "";

    // receive message
    char* message = { 0 };
    n = recv(ConnectSocket, message, length, 0);
    return message;

}

void sendImage(cv::Mat img) {
    int pixel_number = img.rows * img.cols / 2;

    std::vector<uchar> buf(pixel_number);
    cv::imencode(".jpg", img, buf);

    int length = buf.size();
    std::string length_str = std::to_string(length);
    std::string message_length =
        std::string(size_message_length_ - length_str.length(), '0') + length_str;

    iResult = send(ConnectSocket, message_length.c_str(), size_message_length_, 0);
    if (iResult != SOCKET_ERROR) {
        iResult = send(ConnectSocket, (const char*)buf.data(), length, 0);
        if (iResult == SOCKET_ERROR) {
            wprintf(L"send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
        }
    } 

    //printf("Bytes Sent: %d\n", iResult);
}

int main(int, char**)
{
    cv::Mat frame;
    //--- INITIALIZE VIDEOCAPTURE
    cv::VideoCapture cap;

    //IP Address
    std::string ip_add = "127.0.0.1";

    if (connectServer(ip_add)) {
        // open the default camera using default API
    // cap.open(0);
    // OR advance usage: select any API backend
        int deviceID = 0;             // 0 = open default camera
        int apiID = cv::CAP_ANY;      // 0 = autodetect default API
        // open selected camera using selected API
        cap.open(deviceID, apiID);
        // check if we succeeded
        if (!cap.isOpened()) {
            std::cerr << "ERROR! Unable to open camera\n";
            return -1;
        }
        //--- GRAB AND WRITE LOOP
        std::cout << "Start grabbing" << std::endl
            << "Press any key to terminate" << std::endl;
        for (;;)
        {
            // wait for a new frame from camera and store it into 'frame'
            cap.read(frame);
            // check if we succeeded
            if (frame.empty()) {
                std::cerr << "ERROR! blank frame grabbed\n";
                break;
            }
            sendImage(frame);
                /*
        while (true) {
             
                std::string msg = receiveClient();
                std::cout << "[Server]: " << msg << std::endl;
                if (msg == "end") {
                    break;
                }
            } */
            
        }
        
    }

   // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}