#ifndef BROADCASTER_H
#define BROADCASTER_H

#include <atomic>
#include <string>
#include <sys/socket.h>
#include <thread>

class Broadcaster {
public:
  Broadcaster();
  ~Broadcaster();

  Broadcaster(const Broadcaster &) = delete;
  Broadcaster &operator=(const Broadcaster &) = delete;

  void run();
  void stop();

private:
  void broadcastLoop();
  std::thread broadcastThread;
  std::atomic<bool> running{false};
  char message[100];
  int socketFd{-1};
  std::string broadcastIp;
};

#endif // BROADCASTER_H