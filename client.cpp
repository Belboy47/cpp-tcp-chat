#include <WinSock2.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <ws2tcpip.h>

void sendMessages(SOCKET *clientSocket, std::atomic_bool *connected);
void receiveMessages(SOCKET *clientSocket, std::atomic_bool *connected);
void sendAll(std::string message, SOCKET *clientLoopSocket);
int main() {
  std::atomic_bool connected = true;

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    std::cout << "Wsa Startup Failed.";

  SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in socketServerAddress{};
  socketServerAddress.sin_family = AF_INET;
  socketServerAddress.sin_port = htons(54000);
  inet_pton(AF_INET, "127.0.0.1", &socketServerAddress.sin_addr);
  if (connect(clientSocket, (const sockaddr *)&socketServerAddress, sizeof(socketServerAddress)) == SOCKET_ERROR) {
    std::cout << "Connection Failed!\n";
  } else {
    std::cout << "Connected to the Server\n";
    std::thread receiveMessagesThread(receiveMessages, &clientSocket, &connected);
    std::thread sendMessagesThread(sendMessages, &clientSocket, &connected);

    receiveMessagesThread.join();
    sendMessagesThread.join();
  }
  closesocket(clientSocket);
  WSACleanup();
}

void sendMessages(SOCKET *clientSocket, std::atomic_bool *connected) {
  bool isName = true;

  while (*connected) {
    std::string message;
    if (isName) {
      std::cout << "Enter you Name: ";
      std::getline(std::cin, message);
      isName = false;
    } else {
      std::cout << "Write your Message:\n";
      std::getline(std::cin, message);
    }
    if (!message.empty()) {
      if (message == "/quit") {
        *connected = false;
        shutdown(*clientSocket, SD_BOTH);
        return;
      } else {
        message.push_back('\n');
        sendAll(message, clientSocket);
      }
    }
  }
}

void receiveMessages(SOCKET *clientSocket, std::atomic_bool *connected) {
  std::string received;
  while (*connected) {
    std::string message;
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    int rcvBytes = recv(*clientSocket, buffer, sizeof(buffer), 0);
    if (rcvBytes > 0) {
      received.append(buffer, rcvBytes);

      size_t pos;
      while ((pos = received.find('\n')) != std::string::npos) {
        message = received.substr(0, pos);
        received.erase(0, pos + 1);
        std::cout << message << '\n';
      }
    } else if (rcvBytes == 0) {
      std::cout << "Connection Closed";
      *connected = false;
    } else {
      *connected = false;
      std::cout << "Error";
    }
  }
}

void sendAll(std::string message, SOCKET *clientLoopSocket) {
  int sent{};
  while (sent < message.size()) {
    int sentTemp = send(*clientLoopSocket, message.c_str() + sent, message.size() - sent, 0);
    if (sentTemp == SOCKET_ERROR) {
      std::cout << "Sending Failed";
      break;
    }
    if (sentTemp == 0) {
      std::cout << "Client Gracefuly Stopped.";
      break;
    } else
      sent += sentTemp;
  }
}