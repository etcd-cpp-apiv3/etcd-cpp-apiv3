#include <iomanip>
#include "etcd/Value.hpp"
#include "etcd/v3/KeyValue.hpp"

etcd::Value::Value()
  : dir(false),
    created(0),
    modified(0),
    _ttl(0),
    leaseId(0)
{
}


etcd::Value::Value(etcdv3::KeyValue const & kv)
{
  dir=false;
  _key=kv.kvs.key();
  value=kv.kvs.value();
  created=kv.kvs.create_revision();
  modified=kv.kvs.mod_revision();
  leaseId = kv.kvs.lease();
  _ttl = kv.get_ttl();
}

std::string const & etcd::Value::key() const
{
  return _key;
}

bool etcd::Value::is_dir() const
{
  return dir;
}

std::string const & etcd::Value::as_string() const
{
  return value;
}

int etcd::Value::created_index() const
{
  return created;
}

int etcd::Value::modified_index() const
{
  return modified;
}

int etcd::Value::ttl() const
{
  return _ttl;
}

int64_t etcd::Value::lease() const
{
  return leaseId;
}


