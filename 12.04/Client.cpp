#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

using namespace std;

const int PORT = 8888;
const string SERVER = "127.0.0.1";

void send_messages(int socket) {
    string msg;
    while (true) {
        getline(cin, msg);
        if (msg == "/exit") {
            break;
        }
        send(socket, msg.c_str(), msg.size() + 1, 0);
    }
}

void receive_messages(int socket) {
    char buffer[1024];
    int bytesread;
    while ((bytesread = recv(socket, buffer, 1024, 0)) > 0) {
        buffer[bytesread] = 0;
        cout << buffer << endl;
    }
}

int main() {
    setlocale(LC_ALL, "");
#ifdef _WIN32
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int clientsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientsocket == -1) {
        cerr << "Ошибка создания сокета!" << endl;
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER.c_str());
    addr.sin_port = htons(PORT);

    if (connect(clientsocket, (sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "Ошибка подключения к серверу!" << endl;
        return 1;
    }

    cout << "Подключение к серверу успешно!" << endl;

    thread sender(send_messages, clientsocket);
    thread receiver(receive_messages, clientsocket);

    sender.join();
    receiver.join();

    closesocket(clientsocket);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}