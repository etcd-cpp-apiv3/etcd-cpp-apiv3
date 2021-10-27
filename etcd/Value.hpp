#ifndef __ETCD_VECTOR_HPP__
#define __ETCD_VECTOR_HPP__

#include <string>
#include <vector>

namespace etcdv3 {
  class KeyValue;
}

namespace mvccpb {
  class KeyValue;
  class Event;
}

namespace electionpb {
  class LeaderKey;
}

namespace etcd
{
  class Value;
  class Event;
  class Response;

  /**
   * Represents a value object received from the etcd server
   */
  class Value
  {
  public:
    /**
     * Returns true if this value represents a directory on the server. If true the as_string()
     * method is meaningless.
     */
    bool is_dir() const;

    /**
     * Returns the key of this value as an "absolute path".
     */
    std::string const & key() const;

    /**
     * Returns the string representation of the value
     */
    std::string const & as_string() const;

    /**
     * Returns the creation index of this value.
     */
    int64_t created_index() const;

    /**
     * Returns the last modification's index of this value.
     */
    int64_t modified_index() const;

    /**
     * Returns the version of this value.
     */
    int64_t version() const;

    /**
     * Returns the ttl of this value or 0 if ttl is not set
     */
    int ttl() const;
  
    int64_t lease() const;

  protected:
    friend class Response;
    friend class BaseResponse; //deliberately done since Value class will be removed during full V3
    friend class DeleteRpcResponse;
    friend class AsyncDeleteResponse;

    friend class Event;

    Value();
    Value(etcdv3::KeyValue const & kvs);
    Value(mvccpb::KeyValue const & kvs);
    std::string _key;
    bool        dir;
    std::string value;
    int64_t         created;
    int64_t         modified;
    int64_t         _version;
    int         _ttl;
    int64_t     leaseId;
  };

  typedef std::vector<Value> Values;

  class Event
  {
  public:
    enum class EventType {
      PUT,
      DELETE_,
      INVALID,
    };

    enum EventType event_type() const;

    bool has_kv() const;

    bool has_prev_kv() const;

    const Value &kv() const;

    const Value &prev_kv() const;

  protected:
    friend class Response;

    Event(mvccpb::Event const & event);

  private:
    enum EventType event_type_;
    Value _kv, _prev_kv;
    bool _has_kv, _has_prev_kv;
  };

  typedef std::vector<Event> Events;
}

#endif
