#!/bin/bash
##############################################################################
# Copyright (c) 2021 by Rambus, Inc. and/or its subsidiaries
# All rights reserved. Unauthorized use (including, without limitation,
# distribution and copying) is strictly prohibited. All use requires,
# and is subject to, explicit written authorization and nondisclosure
# Rambus, Inc. and/or its subsidiaries
#
# For more information or support, please go to our online support system at
# https://sipsupport.rambus.com.
# In case you do not have an account for this system, please send an e-mail
# to sipsupport@rambus.com.
##############################################################################

# This script runs all the PKA EIP 28 openssl engine tests.
#
# Usage:
#    run:    bash test-all.sh
#
# Date:        12/8/2021

# get script directory (current directory)
TEST_DIR="$(dirname "$(readlink -f "$0")")"

if [ "$1" = "-v" ]; then
echo "yup"
    VALGRIND_PARM=$1
fi

# run all test scripts
bash $TEST_DIR/edcsa-sign-and-verify.sh $VALGRIND_PARM
bash $TEST_DIR/sm2-sign-and-verify.sh $VALGRIND_PARM
bash $TEST_DIR/sm2-encrypt-and-decrypt.sh $VALGRIND_PARM
bash $TEST_DIR/dh-key-agreement.sh $VALGRIND_PARM
bash $TEST_DIR/ecdh-key-agreement.sh $VALGRIND_PARM
bash $TEST_DIR/x25519-key-agreement.sh $VALGRIND_PARM
bash $TEST_DIR/rsa-encrypt-decrypt-sign-verify.sh $VALGRIND_PARM
