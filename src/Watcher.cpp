#include "etcd/Watcher.hpp"

etcd::Watcher::Watcher(std::string const & address, std::string const & key, std::function<void(Response)> callback)
{
  std::string stripped_address(address);
  std::string substr("http://");
  std::string::size_type i = stripped_address.find(substr);
  if(i != std::string::npos)
  {
    stripped_address.erase(i,substr.length());
  }
  std::shared_ptr<Channel> channel = grpc::CreateChannel(stripped_address, grpc::InsecureChannelCredentials());
  watchServiceStub= Watch::NewStub(channel);

  doWatch(key, callback);
}

etcd::Watcher::~Watcher()
{
  call->CancelWatch();
  currentTask.wait();
}

void etcd::Watcher::Cancel()
{
  call->CancelWatch();
  currentTask.wait();
}

void etcd::Watcher::AddKey(std::string const & key)
{
  call->WatchReq(key);
}

void etcd::Watcher::doWatch(std::string const & key, std::function<void(Response)> callback)
{

  call.reset(new etcdv3::AsyncWatchAction(key,true,NULL,watchServiceStub.get()));

  currentTask = pplx::task<void>([this, callback]()
  {  
    return call->waitForResponse(callback);
  });


  //return Response::create(call);

  /*currentTask = client.request(web::http::methods::GET, uri.to_string(), cancellation_source.get_token())
    .then([this](pplx::task<web::http::http_response> response_task)
          {
            try
            {
              auto http_response = response_task.get();
              auto json_task = http_response.extract_json();
              auto json_value = json_task.get();
              callback(etcd::Response(http_response, json_value));
            }
            catch (std::exception const & ex)
            {
              if (pplx::is_task_cancellation_requested() || (ex.what() == std::string("Operation canceled")))
                return;

              if(ex.what() != std::string("Retrieving message chunk header"))
                throw;
            }
            doWatch();
          });*/
}
