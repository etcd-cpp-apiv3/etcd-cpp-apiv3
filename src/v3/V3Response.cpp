#include "etcd/v3/V3Response.hpp"
#include "etcd/v3/action_constants.hpp"

void etcdv3::V3Response::set_error_code(int code) 
{
  error_code = code;
}

void etcdv3::V3Response::set_error_message(std::string msg)
{
  error_message = msg;
}

int etcdv3::V3Response::get_index() const
{
  return index;
}

std::string const & etcdv3::V3Response::get_action() const
{
  return action;
}

int etcdv3::V3Response::get_error_code() const
{
  return error_code;
}

std::string const & etcdv3::V3Response::get_error_message() const
{
  return error_message;
}

void etcdv3::V3Response::set_action(std::string action)
{
  this->action = action;
}

std::vector<etcdv3::KeyValue> const & etcdv3::V3Response::get_values() const
{
  return values;
}

std::vector<etcdv3::KeyValue> const & etcdv3::V3Response::get_prev_values() const
{
  return prev_values;
}

std::vector<int64_t> const& etcdv3::V3Response::get_leases() const {
  return leases;
}

etcdv3::KeyValue const & etcdv3::V3Response::get_value() const
{
  return value;
}

etcdv3::KeyValue const & etcdv3::V3Response::get_prev_value() const
{
  return prev_value;
}

etcdv3::LeaseInfo const& etcdv3::V3Response::get_leaseinfo() const {
  return leaseinfo;
}

bool etcdv3::V3Response::has_values() const
{
  return values.size() > 0;
}

bool etcdv3::V3Response::has_leases() const {
  return leases.size() > 0;
}

void etcdv3::V3Response::set_lock_key(std::string const &key) {
  this->lock_key = key;
}

std::string const & etcdv3::V3Response::get_lock_key() const {
  return this->lock_key;
}

std::vector<mvccpb::Event> const & etcdv3::V3Response::get_events() const {
  return this->events;
}
