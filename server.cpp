#include <WinSock2.h>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <ws2tcpip.h>
//----------------------------------------------
enum class ConnectionState { Connecting, Connected, Disconnected };

struct ClientInfo {
  std::string name;
  std ::string ip;
  std::string port;
  SOCKET socket;
  sockaddr_in address;
  std::atomic<ConnectionState> state = ConnectionState::Connecting;
  std::thread thread;
};

void receiveMessages(ClientInfo *client,
                     std::mutex *clientInfosMutex,
                     std::vector<std::unique_ptr<ClientInfo>> *clientInfos,
                     std::condition_variable *cv);
void broadcastMessages(std::vector<std::unique_ptr<ClientInfo>> *clientInfos,
                       ClientInfo *client,
                       std::string message,
                       std::mutex *clientInfosMutex);
void sendAll(std::string message, SOCKET *clientLoopSocket);
int main() {
  //-----------------------------------------------------------Socket Obligatory
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    std::cout << "WSA Startup Failed";
  SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == INVALID_SOCKET)
    std::cout << "Failed to create socket.";
  sockaddr_in socketServerAddress{};
  socketServerAddress.sin_family = AF_INET;
  socketServerAddress.sin_addr.S_un.S_addr = INADDR_ANY;
  socketServerAddress.sin_port = htons(54000);
  if (bind(serverSocket, (const sockaddr *)&socketServerAddress, sizeof(socketServerAddress)) == SOCKET_ERROR)
    std::cout << "Bind Failed";
  if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    std::cout << "Listen Failed";
  std::cout << "Waiting for Connection\n";
  //------------------------------------------------------------Main Thread
  std::vector<std::unique_ptr<ClientInfo>> clientInfos;
  std::condition_variable cv;
  std::mutex clientInfosMutex;

  std::thread listeningForClientsThread = std::thread([&]() {
    while (true) {
      auto tempClient = std::make_unique<ClientInfo>();
      int clientAddressSize = sizeof(tempClient->address);
      tempClient->socket = accept(serverSocket, (sockaddr *)&tempClient->address, &clientAddressSize);
      if ((tempClient->socket == SOCKET_ERROR)) {
        std::cout << "Accept Failed!\n";
      } else {
        std::cout << "User Connected\n";
        tempClient->state = ConnectionState::Connected;
        char buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &tempClient->address.sin_addr.S_un.S_addr, buffer, INET_ADDRSTRLEN);

        tempClient->ip = buffer;
        tempClient->port = std::to_string(ntohs(tempClient->address.sin_port));
        {
          std::lock_guard lock(clientInfosMutex);
          clientInfos.emplace_back(std::move(tempClient));
          clientInfos.back()->thread =
              std::thread(receiveMessages, clientInfos.back().get(), &clientInfosMutex, &clientInfos, &cv);
        }
      }
    }
  });
  while (true) {
    std::unique_lock lock(clientInfosMutex);
    cv.wait(lock, [&] {
      for (auto it = clientInfos.begin(); it != clientInfos.end(); ++it) {
        if ((*it)->state == ConnectionState::Disconnected) {
          return true;
        }
      }
      return false;
    });
    {
      for (auto it = clientInfos.begin(); it != clientInfos.end();) {
        if ((*it)->state == ConnectionState::Disconnected) {
          closesocket((*it)->socket);
          (*it)->thread.join();
          it = clientInfos.erase(it);

        } else
          ++it;
      }
    }
  }
}

void receiveMessages(ClientInfo *client,
                     std::mutex *clientInfosMutex,
                     std::vector<std::unique_ptr<ClientInfo>> *clientInfos,
                     std::condition_variable *cv) {
  bool isName = true;
  std::string received;
  while (client->state == ConnectionState::Connected) {
    char buffer[1024];
    int rcvBytes = recv(client->socket, buffer, sizeof(buffer), 0);
    if (rcvBytes > 0) {
      received.append(buffer, rcvBytes);
      size_t pos;
      while ((pos = received.find('\n')) != std::string::npos) {
        if (isName) {
          std::lock_guard lock(*clientInfosMutex);
          client->name = '[' + received.substr(0, pos) + "] ";
          received.erase(0, pos + 1);
          isName = false;
        } else {
          std::string message = client->name;
          message += received.substr(0, pos);
          message.push_back('\n');
          received.erase(0, pos + 1);
          broadcastMessages(clientInfos, client, message, clientInfosMutex);
        }
      }
    } else if (rcvBytes == 0) {
      std::lock_guard lock(*clientInfosMutex);
      std::cout << "Connection Closed";
      client->state = ConnectionState::Disconnected;
      cv->notify_one();
    } else {
      std::lock_guard lock(*clientInfosMutex);
      std::cout << "Client Terminated";
      client->state = ConnectionState::Disconnected;
      cv->notify_one();
    }
  }
}

void broadcastMessages(std::vector<std::unique_ptr<ClientInfo>> *clientInfos,
                       ClientInfo *client,
                       std::string message,
                       std::mutex *clientInfosMutex) {
  // One client could hold back the entire Mutex
  std::lock_guard lock(*clientInfosMutex);
  for (auto it = clientInfos->begin(); it != clientInfos->end(); ++it) {
    if ((*it)->socket != client->socket)
      sendAll(message, &((*it)->socket));
    // TODO: If send() detects a disconnected client,
    // mark its state as Disconnected and notify cleanup.
  }
}

void sendAll(std::string message, SOCKET *clientLoopSocket) {
  int sent{};
  while (sent < message.size()) {
    int sentTemp = send(*clientLoopSocket, message.c_str() + sent, message.size() - sent, 0);

    if (sentTemp == SOCKET_ERROR) {
      std::cout << "Sending Failed";
      break;
    } else
      sent += sentTemp;
  }
}