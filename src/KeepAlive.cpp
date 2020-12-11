/**
 * file: KeepAlive.cpp
 * credit to https://github.com/grpc/grpc/pull/8934 for an example on using grpc
 * bidi in c++
 */
#include "etcd/KeepAlive.hpp"

#include "etcd/v3/AsyncLeaseKeepAliveAction.hpp"
#include "etcd/v3/AsyncLeaseRevokeAction.hpp"

etcd::KeepAlive::KeepAlive(Client &client)
    : client_(client) { // , strand_(boost::asio::make_strand(io_)) {
  stub_ = etcdserverpb::Lease::NewStub(client.channel);

  // Open the grpc bidi stream
  stream_ = stub_->AsyncLeaseKeepAlive(&context_, &cq_,
                                       reinterpret_cast<void *>(Type::CONNECT));

  // Start up the long-running task that will manage the stream
  current_task_ = pplx::task<void>([this]() {
    // Keep the io context running even without any work (otherwise it exits
    // immediately before any timers are started)
    boost::asio::executor_work_guard<decltype(io_.get_executor())> work{
        io_.get_executor()};
    // spin up a thread that runs the boost io_context for our timer
    t_ = std::unique_ptr<boost::thread>(
        new boost::thread(boost::bind(&boost::asio::io_context::run, &io_)));

    bool shutdown = false;
    while (!shutdown) {
      void *got_tag;
      bool ok = false;
// Block until the next result is available in the completion queue
// "cq". The return value of Next should always be checked. This
// return value tells us whether there is any kind of event or the cq_
// is shutting down.
#if DEBUG
      std::cout << "Waiting for next event..." << std::endl;
#endif
      if (!cq_.Next(&got_tag, &ok)) {
#if DEBUG
        std::cerr << "Client stream closed. Quitting" << std::endl;
#endif
        break;
      }

      // It's important to process all tags even if the ok is false. One
      // might want to deallocate memory that has be reinterpret_cast'ed to
      // void* when the tag got initialized. For our example, we cast an int
      // to a void*, so we don't have extra memory management to take care
      // of.
      if (ok) {
#if DEBUG
        std::cout << std::endl
                  << "**** Processing completion queue tag " << got_tag
                  << std::endl;
#endif
        switch (static_cast<Type>(reinterpret_cast<int64_t>(got_tag))) {
        case Type::READ:
#if DEBUG
          std::cout << "Read a new message, sending next." << std::endl;
#endif
          sendNextKeepAlive(stream_);
          break;
        case Type::WRITE:
#if DEBUG
          std::cout << "Sent message (async), attempting to read response."
                    << std::endl;
#endif
          readNextMessage(stream_);
          break;
        case Type::CONNECT:
#if DEBUG
          std::cout << "Server connected." << std::endl;
#endif
          break;
        case Type::WRITES_DONE:
#if DEBUG
          std::cout << "Server disconnecting." << std::endl;
#endif
          stream_->Finish(&finish_status_,
                          reinterpret_cast<void *>(Type::FINISH));
          break;
        case Type::FINISH:
#if DEBUG
          std::cout << "Client finish; status = "
                    << (finish_status_.ok() ? "ok" : "cancelled") << std::endl;
#endif
          context_.TryCancel();
          cq_.Shutdown();
          io_.stop();
#if DEBUG
          std::cout << "Cleanup complete." << std::endl;
#endif
          shutdown = true;
          break;
        default:
#if DEBUG
          std::cerr << "Unexpected tag " << got_tag << std::endl;
#endif
          break;
        }
      }
    }
  });
}

void etcd::KeepAlive::queueKeepAlivesCallback(
    const boost::system::error_code &error, KeepAliveTimer *timer) {
#if DEBUG
  std::time_t time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::cout << " ** Queueing keepalives: " << std::ctime(&time);
#endif

  if (error == boost::asio::error::operation_aborted) {
#if DEBUG
    std::cout << " ** Aborting" << std::endl;
#endif
    return;
  }

  // Lock the mutex and copy the lease id's to a queue for processing
  {
    std::lock_guard<std::mutex> lg(mutex_);
    std::map<int64_t, std::shared_ptr<KeepAliveTimer>>::iterator itr;
    for (itr = leases_.begin(); itr != leases_.end(); ++itr) {
      if (itr->second.get() == timer) {
#if DEBUG
        std::cout << " ** Queueing keepalive lease: " << itr->first
                  << std::endl;
#endif
        keepalive_queue_.push(itr->first);
      }
    }
  }

  // Trigger the timer to repeat
  timer->repeat();

  // Kick off the queue processing by sending the first keepalive command
  // You can have at most one write or at most one read at any given time,
  // so we are not sending another keepalive on the stream until the previous
  // one responds
  sendNextKeepAlive(stream_);
}

void etcd::KeepAlive::add(int64_t leaseid,
                          boost::asio::chrono::milliseconds refresh_interval) {
  std::lock_guard<std::mutex> lg(mutex_);

  // See if this lease is already being handled
  auto leases_search = leases_.find(leaseid);
  if (leases_search != leases_.end()) {
    // Lease already exists
    return;
  }

  // Look to see if we already have a timer with this interval
  auto search = timers_.find(refresh_interval);
  if (search != timers_.end()) {
    // Existing timer, add this lease to the timer
    // std::cout << "Found timer with refresh interval: " <<
    // search->first.count() << std::endl;
    leases_.insert(std::make_pair(leaseid, search->second));

  } else {
    // No existing timer, create one and add this lease
    // std::cout << "No timer found with refresh interval: " <<
    // refresh_interval.count() << std::endl;
    auto timer = std::make_shared<KeepAliveTimer>(io_, refresh_interval);
    timer->setCallback(boost::bind(&KeepAlive::queueKeepAlivesCallback, this,
                                   boost::asio::placeholders::error,
                                   timer.get()));
    timer->start();
    timers_.insert(std::make_pair(refresh_interval, timer));
    leases_.insert(std::make_pair(leaseid, timer));
  }
}

void etcd::KeepAlive::remove(int64_t leaseid, bool revoke) {
  {
    std::lock_guard<std::mutex> lg(mutex_);

    // Search for the leaseid
    auto lease = leases_.find(leaseid);
    if (lease != leases_.end()) {
      // Lease exists, let's remove it
      // Check if the timer has any other leases attached or if it needs to be
      // removed
      bool delete_timer = true;
      for (auto timer_search = leases_.begin(); timer_search != leases_.end();
           ++timer_search) {
        if (timer_search->second == lease->second &&
            timer_search->first != lease->first) {
          // Found another lease using this timer, so don't delete the timer
          delete_timer = false;
          break;
        }
      }
      // If we need to delete the timer...
      if (delete_timer) {
        // First make sure to stop the timer.
        lease->second->stop();
        // Find the timer in our timers_ map
        for (auto timer_search = timers_.begin(); timer_search != timers_.end();
             ++timer_search) {
          if (timer_search->second == lease->second) {
            timers_.erase(timer_search);
            break;
          }
        }
      }
      // Finally, we can erase the lease entry
      leases_.erase(leaseid);
    } // else = no record of the lease
  }

  if (revoke) {
    client_.leaserevoke(leaseid);
  }
}

void etcd::KeepAlive::sendNextKeepAlive(
    std::unique_ptr<
        grpc::ClientAsyncReaderWriter<etcdserverpb::LeaseKeepAliveRequest,
                                      etcdserverpb::LeaseKeepAliveResponse>>
        &stream) {
  LeaseKeepAliveRequest request;

  {
    std::lock_guard<std::mutex> lg(mutex_);
    if (!keepalive_queue_.empty()) {
      int64_t leaseid = keepalive_queue_.front();
      keepalive_queue_.pop();
#if DEBUG
      std::cout << "sending keepalive for " << leaseid << std::endl;
#endif
      request.set_id(leaseid);
    } else {
      // std::cout << "no keepalives left" << std::endl;
      return;
    }
  }

  stream->Write(request, reinterpret_cast<void *>(Type::WRITE));
}

void etcd::KeepAlive::readNextMessage(
    std::unique_ptr<
        grpc::ClientAsyncReaderWriter<etcdserverpb::LeaseKeepAliveRequest,
                                      etcdserverpb::LeaseKeepAliveResponse>>
        &stream) {
  // std::cout << " ** Got response: " << response_.ttl() << std::endl;
  stream->Read(&response_, reinterpret_cast<void *>(Type::READ));
}

void etcd::KeepAlive::removeAll(bool revoke) {
  {
    // empty the queue
    std::lock_guard<std::mutex> lg(mutex_);
    std::queue<int64_t> empty;
    std::swap(keepalive_queue_, empty);
  }

  if (revoke) {
    // revoke each lease
    for (auto lease : leases_) {
      client_.leaserevoke(lease.first);
    }
  }
  // Clear the maps, which will destroy the timers as the shared_ptr's go out of
  // scope
  std::lock_guard<std::mutex> lg(mutex_);
  leases_.clear();
  timers_.clear();
}

etcd::KeepAlive::~KeepAlive() {
  removeAll();
  stream_->WritesDone(reinterpret_cast<void *>(Type::WRITES_DONE));
  try {
    t_->join();
  } catch (const boost::thread_interrupted &) { /* suppress */
  };
}
