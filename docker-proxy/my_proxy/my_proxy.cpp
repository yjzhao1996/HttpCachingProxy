#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <thread>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <pthread.h>
#include <ostream>
#include <limits.h>
#include <time.h>
#include <unordered_map>
#include <cstdlib>
#include <fstream>
#include "proxy.h"


using namespace std;

void thread_helper(Proxy proxy){
  proxy.initial_process();
}



int main(){
  cout<<"start!!!!"<<endl;
  //BEGIN_REF - cited from ECE650 TCP demo code
  const char *hostname = NULL;
  const char *port     = "8080";
  int socket_fd;
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  status=listen(socket_fd,1500);
  //END_REF
  int id=1;
  while(1){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd=accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if(client_connection_fd==-1){
      //fail to accept connection request
      log_error(id,1);
      continue;
    }
    //BEGIN_REF - https://blog.csdn.net/ma2595162349/article/details/78726916
    string ip=inet_ntoa(((struct sockaddr_in*)&socket_addr)->sin_addr);
    //END_REF
    Proxy proxy(socket_fd,client_connection_fd,id,ip);
    thread(thread_helper,proxy).detach();
    id++;
  }
  close(socket_fd);
  return 0;
}
