
#include "parser.h"
#include "function.h"

using namespace std;

class Cache{
public:
  unordered_map<string,response_data> cache;

  Cache(){
    cache.clear();
  }
  
  void save(string req, int size, vector<char> response, int ID){
    Parser P(response);
    if(!P.resp_parse_200ok()){
      ofstream ofs;
      ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
      ofs<<ID<<": "<<"not cacheable because don't recieve 200 OK "<<endl;
      ofs.close();
      return;
    }
    if(P.resp_parse_nostore()){
      ofstream ofs;
      ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
      ofs<<ID<<": "<<"not cacheable because no-store "<<endl;
      ofs.close();
      return;
    }
    if(cache.size()>200){
      cache.erase(cache.begin()); //replacement policy:random;
    } 
    response_data response_block;
    
    //********I want add a response max-age detection function here but I
    //dont't know how to transfer seconds to a date time gracefully*****//
    
    //if(P.resp_parse_c_ctrl().find("max-age=")!=string::npos){    
    // response_block.expire_time = 
    //}
    //else{
    response_block.expire_time = P.resp_parse_expire();
    //}
    response_block.create_time = P.resp_parse_date();
    if(P.resp_parse_c_ctrl().find("no-cache")!=string::npos||(P.resp_parse_c_ctrl().find("must-revalidate")!=string::npos)){
      ofstream ofs;
      ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
      ofs<<ID<<": "<<"cached, but requires re-validation"<<endl;
      ofs.close();
      response_block.revalidation = true;
    }
    else{
      response_block.revalidation = false;
    }
    response_block.size = size;
    response_block.data = response;
    response_block.Etag = P.resp_parse_ETag();
    ofstream ofs;
    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
    ofs<<ID<<": "<<"cached, expires at "<<response_block.expire_time<<endl;
    ofs.close();
    //cout<<"save cache"<<endl;
    cache[req] = response_block;
  }
  
  int search(int cliend_fd,string req,vector<char> response,int ID){
    int status;
    Parser P(response);
    string first_line;
    //Etag matches?
    string Etag = P.resp_parse_ETag();
    if(cache.find(req)!=cache.end()){          
      if(cache[req].Etag != Etag){
	ofstream ofs;
	ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	ofs<<ID<<": "<<"in cache, requires validation"<<endl;
	ofs.close();
        //cout<<"requires validation"<<endl;
        return 1;
      }
      time_t create_t=extract_time(cache[req].create_time);
      string expire_time = cache[req].expire_time;
      if(expire_time.empty()){
	string modify_time = P.resp_parse_modify();
	if(modify_time.empty()){
	  ofstream ofs;
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"in cache, valid"<<endl;
	  ofs.close();
	  status=send(cliend_fd,(cache[req].data).data(),cache[req].size,0);
	  if(status==-1){
	    //send fail
	    log_error(ID,5);
	    return 1;
	  }
	  string temp(cache[req].data.begin(),cache[req].data.end());
	  first_line=temp.substr(0,temp.find("\r\n"));
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"Responding "<<first_line <<endl;
	  ofs.close();
	  //cout<<"use cache"<<endl;
	  return 0;
	}
	else{
	  time_t modify_t = extract_time(modify_time);
	  double gap = modify_t - create_t;
	  if(gap > 0){
	    //cout<<"modified at"<<endl;
	    return 1;
	  }
	  //else?
	  else{
	    ofstream ofs;
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"in cache, valid"<<endl;
	    ofs.close();
	    status=send(cliend_fd,(cache[req].data).data(),cache[req].size,0);
	    if(status==-1){
	    //send fail
	      log_error(ID,5);
	    return 1;
	    }
	    string temp(cache[req].data.begin(),cache[req].data.end());
	    first_line=temp.substr(0,temp.find("\r\n"));
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"Responding "<<first_line <<endl;
	    ofs.close();
	    //cout<<"use cache"<<endl;
	    return 0;
	  }
	}
      }
      else{
	time_t expire_t = extract_time(expire_time);
	double gap = expire_t - create_t;
	if(gap < 0){
	  ofstream ofs;
	  ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	  ofs<<ID<<": "<<"in cache, but expired at"<< expire_time<<endl;
	  ofs.close();
	  //cout<<"expired"<<endl;
	  return 1;
	}
	if(gap > 0){
	  string modify_time = P.resp_parse_modify();
	  if(modify_time.empty()){        //modify time does not exist, send back
	    ofstream ofs;
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"in cache, valid"<<endl;
	    ofs.close();
	    //cout<<"use cache"<<endl;
	    status=send(cliend_fd,(cache[req].data).data(),cache[req].size,0);
	    if(status==-1){
	    //send fail
	      log_error(ID,5);
	    return 1;
	  }
	    string temp(cache[req].data.begin(),cache[req].data.end());
	    first_line=temp.substr(0,temp.find("\r\n"));
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"Responding "<<first_line <<endl;
	    ofs.close();
	    return 0;
	  } 
	  else{
	    time_t modify_t = extract_time(modify_time);
	    double diff = modify_t - create_t;
	    if(diff >0){
	      cout<<"modified at"<<endl;
	      return 1;
	    }
	    ofstream ofs;
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"in cache, valid"<<endl;
	    ofs.close(); 
	    status=send(cliend_fd,(cache[req].data).data(),cache[req].size,0);
	    if(status==-1){
	    //send fail
	      log_error(ID,5);
	    return 1;
	    }
	    //cout<<"use cache"<<endl;
	    string temp(cache[req].data.begin(),cache[req].data.end());
	    first_line=temp.substr(0,temp.find("\r\n"));
	    ofs.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::app);
	    ofs<<ID<<": "<<"Responding "<<first_line <<endl;
	    ofs.close();
	    return 0;
	  }
	  return 1;
	}
      }
      return 1;
    }
    return 1;
  }
};
