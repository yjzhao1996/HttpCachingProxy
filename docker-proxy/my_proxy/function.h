
using namespace std;

class response_data{
public:
  string expire_time;
  string create_time;
  string Etag;
  bool revalidation;
  int size;
  vector<char> data;
};

void log_error(int id, int err_num){
      ofstream ofs;
      ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
      if(err_num==1){
      ofs<<id<<": "<<"ERROR fail to accept connection request"<<endl;
      }
      else if(err_num==2){
	ofs<<id<<": "<<"ERROR recv() error"<<endl;
      }
      else if(err_num==3){
	ofs<<id<<": "<<"ERROR the format of message is wrong"<<endl;
      }
      else if(err_num==4){
	ofs<<id<<": "<<"ERROR fail to connect to server"<<endl;
      }
      else if(err_num==5){
	ofs<<id<<": "<<"ERROR send() error"<<endl;
      }
      else if(err_num==6){
	ofs<<id<<": "<<"ERROR select() error"<<endl;
      }
      ofs.close();
}

//BEGIN_REF - https://blog.csdn.net/huangpin815/article/details/70495906
time_t extract_time(string time){
  const char* format= time.c_str();
  struct tm s;
  strptime(format, "%a, %d %b %Y %H:%M:%S GMT", &s);
  return mktime(&s);
}
//END_REF

