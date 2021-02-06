#!/bin/bash
#
# generate ca certificate for etcd
#
# referred from: https://github.com/kelseyhightower/etcd-production-setup

set -x
set -e
set -o pipefail

shopt -s extglob

ROOT=$(dirname "${BASH_SOURCE[0]}")

pushd $ROOT

rm -rf !(setup-ca.sh|reset-ca.sh|openssl.cnf)

popd # $ROOT

set +x
set +e
set +o pipefail
