
#include "cache.h"

using namespace std;

Cache my_cache;

class Proxy{
private:
 
  int socket_fd;
  int client_connection_fd;
  int port_num;
  int ID;
  string ip;

  public:
 
  Proxy(int a, int b, int c, string d):socket_fd(a),client_connection_fd(b),ID(c),ip(d){}
  
  void initial_process(){
    vector<char> temp_buff(65536);
    int sum=0;
    int len;
    string check_end;
    do{
      char *p = temp_buff.data();
      p = p+sum;
      len=recv(client_connection_fd,p,65536,0);
      if(len<0){
	//cout<<"recv error"<<endl;
	log_error(ID,2);
	return ;   
      }
      sum += len;
      string check_end(temp_buff.begin(),temp_buff.end());
      if(check_end.find("\r\n\r\n")!=string::npos){
	  break;
      }
      temp_buff.resize(sum+65536);
      //}while(len!=0);
    }while(check_end.find("\r\n\r\n")==string::npos);
    string temp(temp_buff.begin(),temp_buff.end());
    if((temp.find("\r\n\r\n"))==string::npos){
      //cout<<"format error"<<endl;
      log_error(ID,3);
      char err[]="HTTP/1.1 400 Bad Request\r\n\r\n";
      send(client_connection_fd,err,strlen(err),0);
      return;
    }
    //BEGIN_REF - https://www.cnblogs.com/cappuccino/p/5338147.html
    string first_line=temp.substr(0,temp.find("\r\n"));
    time_t rawtime;
    struct tm *ptminfo;
    time(&rawtime);
    ptminfo = localtime(&rawtime);
    //END_REF
    ofstream ofs;
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<first_line <<" from "<<ip<<" @ "<<asctime(ptminfo)<<endl;
    ofs.close();
    
    Parser P(temp_buff);
    port_num=P.req_parse_port();
    string server=P.req_parse_server();
    if(P.req_parse_method()=="CONNECT"){
      //cout<<"connect"<<endl;
      connect_method(server,port_num);
    }
    else if(P.req_parse_method()=="GET"){
      //cout<<"get"<<endl;
      if(my_cache.cache.find(temp)!=my_cache.cache.end()){
        //if forbid using cache
	if(temp.find("no-store")!=string::npos){
	  request_server(temp,server,port_num,true);
	}
	else if((temp.find("no-cache")!=string::npos)||(my_cache.cache[temp].revalidation)){
	  ofstream ofs;
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"in cache, requires validation"<<endl;
	  ofs.close();
	  request_server(temp,server,port_num,false);
	}
	else if(temp.find("max-age")!=string::npos){
	  time_t current;
	  time(&current);
	  time_t creat_t=extract_time(my_cache.cache[temp].create_time);
	  time_t max_age=P.req_parse_age();
	  if(current-creat_t<max_age){
	    look_up_cache(temp,server,port_num);
	  }
	  else{
	    request_server(temp,server,port_num,false);
	  }
	}
	else{
	  look_up_cache(temp,server,port_num);
	}
      }
      else{
	ofstream ofs;
	ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	ofs<<ID<<": "<<"not in cache"<<endl;
	ofs.close();
	request_server(temp,server,port_num,false);
      }
    }
    else if(P.req_parse_method()=="POST"){
      //cout<<"post"<<endl;
      request_server(temp,server,port_num,true);
    }
    else{
      //cout<<P.req_parse_method()<<endl;
    }
    close(client_connection_fd);
  }

  void connect_method(string server,int port_num){ 
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(server.c_str(),to_string(port_num).c_str() , &host_info, &host_info_list);
    socket_fd = socket(host_info_list->ai_family, 
		       host_info_list->ai_socktype, 
		       host_info_list->ai_protocol);
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status==-1){
      //fail to connect
      log_error(ID,4);
      close(socket_fd);
      return;
    }
    char success[]="HTTP/1.1 200 OK\r\n\r\n";
    status=send(client_connection_fd,success,strlen(success),0);
    if(status==-1){
      //fail to send
      log_error(ID,5);
      close(socket_fd);
      return;
    }
    //BEGIN_REF - https://www.cnblogs.com/skyfsm/p/7079458.html
    int select_fd;
    if(socket_fd>client_connection_fd){
      select_fd=socket_fd;
    }
    else{
      select_fd=client_connection_fd;
    }
    fd_set readfds;
    char buffer[65536];
    while(1){
      FD_ZERO(&readfds);
      FD_SET(socket_fd,&readfds);
      FD_SET(client_connection_fd,&readfds);
      int result=select(select_fd+1,&readfds,NULL,NULL,NULL);
      if(result<0){
	log_error(ID,6);
        //error;
	break ;
      }
      memset(buffer,0,65536); 
      int len;
      if(FD_ISSET(client_connection_fd,&readfds)){
	//END_REF
	len=recv(client_connection_fd,buffer,65536,0);
	if(len<0){
	  //recv fail
	  log_error(ID,2);
	  break;
	}
	else if(len==0){
          //transmission complete
	  close(socket_fd);
	  ofstream ofs;
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"Tunnel closed"<<endl;
	  ofs.close();
	  return;
	}
	len=send(socket_fd,buffer,len,0);
	if(len<0){
          //transmission fail?
	  log_error(ID,5);
	  break;
	}
      }
      else if(FD_ISSET(socket_fd,&readfds)){
	len=recv(socket_fd,buffer,65536,0);
	if(len<0){
	  //recv fail
	  log_error(ID,2);
	  break;
	}
	else if(len==0){
          //transmission complete
	  close(socket_fd);
	  ofstream ofs;
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"Tunnel closed"<<endl;
	  ofs.close();
	  return;
	}
	len=send(client_connection_fd,buffer,len,0);
	if(len<0){
          //transmission fail?
	  log_error(ID,5);
	  break;
	}
      }
    }
    close(socket_fd);
    ofstream ofs;
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"Tunnel closed"<<endl;
    ofs.close();
    return;
  }

  void look_up_cache(string request,string server,int port_num){  
    int sum=0;
    int len=0;
    const char* sent=request.c_str();

    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
  
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(server.c_str(),to_string(port_num).c_str() , &host_info, &host_info_list);
    socket_fd = socket(host_info_list->ai_family, 
		       host_info_list->ai_socktype, 
		       host_info_list->ai_protocol);
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status==-1){
      //fail to connect
      log_error(ID,4);
      close(socket_fd);
      return;
    }
    status=send(socket_fd,sent,strlen(sent),0);
    if(status==-1){
      //fail to send
      log_error(ID,5);
      close(socket_fd);
      return;
    }
    //to get the response header.
    vector<char> temp_buff(1000);
    do{
      char *p = temp_buff.data();
      len=recv(socket_fd,p+sum,1000,0);
      if(len<0){
	close(socket_fd);
	return ;   
      }
      sum += len;
      string check(temp_buff.begin(),temp_buff.end());
      if(check.find("\r\n\r\n")!=string::npos){
        break;
      }
      temp_buff.resize(sum+1000);
    }while(len!=0);
    string check(temp_buff.begin(),temp_buff.end());
    if(check.find("\r\n\r\n")==string::npos){
      //wrong format
      log_error(ID,3);
      char err[]="HTTP/1.1 502 Bad Gateway\r\n\r\n";
      send(client_connection_fd,err,strlen(err),0);
      close(socket_fd);
      return;
    }
    close(socket_fd);
    int result = my_cache.search(client_connection_fd,request,temp_buff,ID);
    if(result ==1){//can't use data in cache
      request_server(request,server,port_num,false);
    }
  }
  
  void request_server(string request,string server,int port_num,bool post){
    string first_line=request.substr(0,request.find("\r\n"));
    ofstream ofs;
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"Requesting "<<first_line<<" from "<<server<<endl;
    ofs.close();
    const char* sent=request.c_str();
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(server.c_str(),to_string(port_num).c_str() , &host_info, &host_info_list);
    if(status!=0){
      //error
      cout<<gai_strerror(status)<<endl;
      return;
    }
    socket_fd = socket(host_info_list->ai_family, 
		       host_info_list->ai_socktype, 
		       host_info_list->ai_protocol);
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status==-1){
      //fail to connect
      log_error(ID,4);
      close(socket_fd);
      return;
    }
    status=send(socket_fd,sent,strlen(sent),0);
    if(status==-1){
      //fail to send
      log_error(ID,5);
      close(socket_fd);
      return;
    }
    vector<char> temp_buff(65536);
    int sum=0;
    int len;
    int size = 65536;
    int end = 0;
    do{
      char *p = temp_buff.data();
      len=recv(socket_fd,p+sum,size,0);
      if(len<0){
	//error
	log_error(ID,2);
	close(socket_fd);
      return;   
      }
      sum += len;
      string check_chunked(temp_buff.begin(),temp_buff.end());
      //transfer encoding:chunked?
      if(check_chunked.find("chunked")!=string::npos){
	//cout<<"start process chunked"<<endl;
	process_chunked(temp_buff,socket_fd,len,server);
	return;
      }
      Parser P(temp_buff);
      //response content length, and resize buffer
      if((P.resp_parse_len())>=0){
	end = P.resp_parse_len()+P.resp_parse_header_len();
	temp_buff.resize(end);
        size = end - sum;//
      }
      else{
	temp_buff.resize(sum+65536);
      }
      //cout<<ID<<" "<<sum<<" "<<end<<endl;
      if(sum == end){
        break;
      }
    }while(len!=0);
    string check(temp_buff.begin(),temp_buff.end());
    
    if(check.find("\r\n\r\n")==string::npos){
      //wrong format
      log_error(ID,3);
      char err[]="HTTP/1.1 502 Bad Gateway\r\n\r\n";
      send(client_connection_fd,err,strlen(err),0);
      close(socket_fd);
      return;
    }

    first_line=check.substr(0,check.find("\r\n"));
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"Received "<<first_line<<" from "<<server<<endl;
    ofs.close();
    
    if(!post){
    my_cache.save(request,sum,temp_buff,ID);
    }
    const char *resp = temp_buff.data();
    status=send(client_connection_fd,resp,sum,0);
    if(status==-1){
      //cout<<"send fail"<<endl;
      log_error(ID,5);
      close(socket_fd);
      return;
    }
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"Responding "<<first_line<<endl;
    ofs.close();
    close(socket_fd);
  }

  void process_chunked(vector<char>data,int fd,int len,string server){
    int status;
    string temp(data.begin(),data.end());
    string first_line=temp.substr(0,temp.find("\r\n"));
    ofstream ofs;
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"Received "<<first_line<<" from "<<server<<endl;
    ofs<<ID<<": "<<"Responding "<<first_line<<endl;
    ofs.close();
    status=send(client_connection_fd,data.data(),len,0);
    if(status==-1){
      //fail to send
      log_error(ID,5);
      close(fd);
      return;
    }
    string check(data.begin(),data.end());
    vector<char> temp_buff(65536);
    int length=0;
    while(1){
      if(check.find("0\r\n\r\n")!=string::npos){
	//the last chunk
	break;
      }
      memset(temp_buff.data(),0,65536);
      length=recv(fd,temp_buff.data(),65536,0);
      if(length<0){
	//recv error
	log_error(ID,2);
	break;
      }
      status=send(client_connection_fd,temp_buff.data(),length,0);
      if(status==-1){
	//cout<<"send fail"<<endl;
	log_error(ID,5);
	break;
      }
      //cout<<"send chunk"<<endl;
      string app(temp_buff.begin(),temp_buff.end());
      check.append(app);
    }
    close(fd);
    return;
  }
};
