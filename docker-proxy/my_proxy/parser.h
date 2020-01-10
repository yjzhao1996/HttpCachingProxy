

using namespace std;

class Parser{
public:
  vector<char> message;

  Parser(vector<char> m):message(m){}
  
  string req_parse_server(){
    string request(message.begin(),message.end());
    request = request.substr(request.find("Host: ")+6);
    string server = request.substr(0,request.find("\r\n"));
    if(server.find(":")!=string::npos){
      server = server.substr(0,server.find(":"));
    }
    return server;
  }
  
  int req_parse_port(){
    string request(message.begin(),message.end());
    int port=80;
    request = request.substr(request.find("Host:")+6);
    request = request.substr(0,request.find("\r\n"));
    if(request.find(":")!=string::npos){
      request = request.substr(request.find(":")+1);
      string temp = request.substr(0,request.find("\r\n"));
      port = stoi(temp);
    }
    return port;
  }
  
  string req_parse_method(){
    string request(message.begin(),message.end());
    string method = request.substr(0,request.find(" "));
    return method;
  }

  time_t req_parse_age(){
    string request(message.begin(),message.end());
    request = request.substr(request.find("max-age=")+8);
    string age = request.substr(0,request.find("\r\n"));
    time_t max_age=stoi(age);
    return max_age;
  }

  bool resp_parse_200ok(){
    string response(message.begin(),message.end());
    if(response.find("200 OK")!=string::npos){
      return true;
    }
    return false;
  }

  bool resp_parse_nostore(){
    string response(message.begin(),message.end());
    if(response.find("no-store")!=string::npos){
      return true;
    }
    return false;
  }

  string resp_parse_date(){
    string response(message.begin(),message.end());
    string time;
    if(response.find("Date:")!=string::npos){
      response = response.substr(response.find("Date:")+6);
      time = response.substr(0,response.find("\r\n"));
    }
    return time;
  }

  string resp_parse_expire(){
    string response(message.begin(),message.end());
    string time;
    if(response.find("Expires:")!=string::npos){
      response = response.substr(response.find("Expires:")+9);
      time = response.substr(0,response.find("\r\n"));
    }
    return time;
  }

  string resp_parse_modify(){
    string response(message.begin(),message.end());
    string time;
    if(response.find("Last-Modified:")!=string::npos){
      response = response.substr(response.find("Last-Modified:")+15);
      time = response.substr(0,response.find("\r\n"));
    }
    return time;
  }

  string resp_parse_ETag(){
    string response(message.begin(),message.end());
    string Etag;
    if(response.find("ETag:")!=string::npos){
      response = response.substr(response.find("ETag:")+6);
      Etag = response.substr(0,response.find("\r\n"));
    }
    return Etag;
  }

  int resp_parse_len(){
    string response(message.begin(),message.end());
    int length = -1;
    if(response.find("Content-Length:")!=string::npos){
      response = response.substr(response.find("Content-Length:")+16);
      response = response.substr(0,response.find("\r\n"));
      length = stoi(response,NULL,10);
    }
    return length;
  }

  int resp_parse_header_len(){
    string response(message.begin(),message.end());
    response = response.substr(0,response.find("\r\n\r\n"));
    int len = response.size()+4;
    return len;
  }

  string resp_parse_c_ctrl(){
    string response(message.begin(),message.end());
    string c_ctrl;
    if(response.find("Cache-Control:")!=string::npos){
      response = response.substr(response.find("Cache-Control:")+15);
      c_ctrl = response.substr(0,response.find("\r\n"));
    }
    return c_ctrl;
  }
};
