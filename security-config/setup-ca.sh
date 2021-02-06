#!/bin/bash
#
# generate ca certificate for etcd
#
# referred from: https://github.com/kelseyhightower/etcd-production-setup

set -x
set -e
set -o pipefail

ROOT=$(dirname "${BASH_SOURCE[0]}")

pushd $ROOT

touch index.txt
echo '01' > serial

mkdir -p private
mkdir -p certs
mkdir -p newcerts

# Create the CA Certificate and Key
openssl req -config ./openssl.cnf -new -x509 -extensions v3_ca \
  -keyout private/ca.key -out certs/ca.crt \
  -passin pass:etcd-ca -passout pass:etcd-ca \
  -subj "/C=US/ST=CA/L=CA/O=etcd-ca/CN=ca.etcd.example.com/emailAddress=ca.etcd.example.com"

# Verify the CA Certificate
openssl x509 -in certs/ca.crt -noout -text

# Create an etcd server certificate
# If you want cert verification to work with IPs in addition to hostnames, be sure to set the SAN env var:
# export SAN="IP:127.0.0.1, IP:10.0.1.10"
export SAN="IP:127.0.0.1"

openssl req -config openssl.cnf -new -nodes \
  -keyout private/etcd0.example.com.key -out etcd0.example.com.csr \
  -subj "/C=US/ST=CA/L=CA/O=etcd-ca/CN=etcd0.example.com/emailAddress=ca.etcd.example.com"

# Sign the cert
openssl ca -batch -config openssl.cnf -extensions etcd_server \
  -passin pass:etcd-ca \
  -keyfile private/ca.key \
  -cert certs/ca.crt \
  -out certs/etcd0.example.com.crt -infiles etcd0.example.com.csr

# Verify the etcd Server Certificate
openssl x509 -in certs/etcd0.example.com.crt -noout -text

# Create an etcd client certificate
unset SAN

openssl req -config openssl.cnf -new -nodes \
  -keyout private/etcd-client.key -out etcd-client.csr \
  -subj "/C=US/ST=CA/L=CA/O=etcd-ca/CN=etcd_client/emailAddress=ca.etcd.example.com"

openssl ca -batch -config openssl.cnf -extensions etcd_client \
  -passin pass:etcd-ca \
  -keyfile private/ca.key \
  -cert certs/ca.crt \
  -out certs/etcd-client.crt -infiles etcd-client.csr


# Configuring etcd for SSL

# Configure etcd

# $ etcd --advertise-client-urls https://etcd0.example.com:2379 \
#   --listen-client-urls https://10.0.1.10:2379 \
#   --cert-file etcd0.example.com.crt \
#   --key-file etcd0.example.com.key

# Configuring etcd clients for SSL

# cURL
# $ curl --cacert ca.crt -XPUT -v https://etcd0.example.com:2379/v2/keys/foo -d value=bar
# $ curl --cacert ca.crt -v https://etcd0.example.com:2379/v2/keys

# etcdctl
# $ etcdctl -C https://etcd0.example.com:2379 --ca-file ca.crt set foo bar 
# $ etcdctl -C https://etcd0.example.com:2379 --ca-file ca.crt get foo

# Configuring etcd for client auth
# $ etcd --advertise-client-urls https://etcd0.example.com:2379 \
#   --listen-client-urls https://10.0.1.10:2379 \
#   --cert-file etcd0.example.com.crt \
#   --key-file etcd0.example.com.key \
#   --client-cert-auth --trusted-ca-file ca.crt \
#
# Notice the usage of the `--client-cert-auth` and `--trusted-ca-file` flag. This is what enables client auth.

# Configuring etcd clients for client auth

# etcdctl
# $ etcdctl -C https://etcd0.example.com:2379 \
#   --cert etcd-client.crt \
#   --key etcd-client.key \
#   --cacert ca.crt \
#   get foo

popd # $ROOT

set +x
set +e
set +o pipefail
