#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <set>
#include <fstream>

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
const int MAXCLIENTS = 10;
const string HISTORY_FILE = "chat_history.txt";

typedef vector<string> msg_history;
typedef set<int> connected_clients;
typedef struct {
    int socket;
    thread* threadptr;
} client_info;

msg_history messages;
connected_clients clients;
mutex mtx;

void load_history() {
    ifstream file(HISTORY_FILE);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            messages.push_back(line);
        }
        file.close();
    }
}

void save_history() {
    ofstream file(HISTORY_FILE);
    if (file.is_open()) {
        for (const string& msg : messages) {
            file << msg << endl;
        }
        file.close();
    }
}

void broadcast_message(const string& msg, int sender) {
    for (int c : clients) {
        if (c != sender) {
            send(c, msg.c_str(), msg.size() + 1, 0);
        }
    }
}

void broadcast_notification(const string& notification) {
    for (int c : clients) {
        send(c, notification.c_str(), notification.size() + 1, 0);
    }
}

void client_handler(int clientsocket) {
    char buffer[1024];
    int bytesread;

    mtx.lock();
    for (const string& msg : messages) {
        send(clientsocket, msg.c_str(), msg.size() + 1, 0);
    }
    mtx.unlock();

    string notification = "Новый клиент подключился!";
    broadcast_notification(notification);

    while ((bytesread = recv(clientsocket, buffer, 1024, 0)) > 0) {
        buffer[bytesread] = 0;
        string msg(buffer);

        mtx.lock();
        messages.push_back(msg);
        broadcast_message(msg, clientsocket);
        save_history();
        mtx.unlock();
    }

    mtx.lock();
    clients.erase(clientsocket);
    notification = "Клиент отключился!";
    broadcast_notification(notification);
    mtx.unlock();

    closesocket(clientsocket);
}

int main() {
    setlocale(LC_ALL, "");

#ifdef _WIN32
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int serversocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serversocket == -1) {
        cerr << "Ошибка создания сокета!" << endl;
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(serversocket, (sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "Ошибка привязки сокета!" << endl;
        return 1;
    }

    if (listen(serversocket, MAXCLIENTS) == -1) {
        cerr << "Ошибка прослушивания сокета!" << endl;
        return 1;
    }

    cout << "Сервер запущен на порту " << PORT << endl;

    load_history();

    while (true) {
        sockaddr_in clientaddr;
        int addrsize = sizeof(clientaddr);
        int clientsocket = accept(serversocket, (sockaddr*)&clientaddr, &addrsize);
        if (clientsocket == -1) {
            cerr << "Ошибка принятия подключения!" << endl;
            continue;
        }

        mtx.lock();
        clients.insert(clientsocket);
        mtx.unlock();

        client_info* ci = new client_info;
        ci->socket = clientsocket;
        ci->threadptr = new thread(client_handler, clientsocket);
    }

    save_history();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}