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
#
# This script tests openssl engine e_eip28pka.so with openssl 1.1.1k 
# running on an x86 host and a PCIe virtex HW with the PKA module.
# Tests:
#     1. ECDSA sign and verify openssl positive flow
#     2. ECDSA sign and verify engine eip28 positive flow
#     3. ECDSA sign and verify openssl negative flow
#     4. ECDSA sign and verify engine eip28 negative flow
# Setup:
# Setup:
# 1. Configure the openssl:        export OPENSSL_DIR="/path/to/openssl-1.1.1k"
# 2. Set engine path:            export OPENSSL_ENGINES=/path/to/engines
# Usage:
#    run all tests:        bash sign-and-verify.sh
#     run specific test:    bash sign-and-verify.sh <test number>
#
# Date:        12/8/2021 
##############################################################################

############################ constants ############################

DEBUG=false
OPENSSL=$OPENSSL_DIR/apps/openssl
VALIDATE_SETUP=true
ENGINE_ID_28=eip28pka
KERNEL_MODULE=umpci_k
PASS=0
FAIL=1
DATESTAMP=$(date +%y%m%d_%H%M)
TEMPFILE=tempfile.txt
VALGRIND_CMD='valgrind --leak-check=full --show-leak-kinds=all --log-file='$TEMPFILE

############################ functions ############################
print_function() {
    echo "${FUNCNAME[1]}"
}

print_openssl_details() {
    [ $DEBUG = true ] && print_function
    which openssl
    ${OPENSSL} version
}

validate_environment() {
    [ $DEBUG = true ] && print_function
    if [ -z $OPENSSL_DIR ]; then
        echo "Error: Environment variable OPENSSL_DIR is undefined"
        return $FAIL
    fi
    if [ ! -f $OPENSSL_DIR/apps/openssl ]; then
        echo "Error: openssl client not found"
        return $FAIL
    fi
    if [ -z $OPENSSL_ENGINES ]; then
        echo "Error: Environment variable OPENSSL_ENGINES is undefined"
        return $FAIL
    fi
    if [ $(lsmod | grep "$KERNEL_MODULE" -c) -ne 1 ]; then
        echo "Error: kernel module $KERNEL_MODULE not found"
        return $FAIL
    fi
    return $PASS
}

print_engine_capabilities() {
    [ $DEBUG = true ] && print_function
    ${OPENSSL} engine -c $ENGINE_ID_28
}

create_test_file() {
    local filename=$1
    local length=$2
    [ $DEBUG = true ] && print_function
    echo $(${OPENSSL} rand -base64 $length) > $filename
    [ $DEBUG = true ] && cat $filename
}

create_ECDSA_private_key() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// create_ECDSA_private_key $2" >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    # possible curves: ${OPENSSL} ecparam -list_curves
    # prev. curve tested: secp384r1
    local ret
    local print
    print=`$valgrind ${OPENSSL} ecparam \
        $parameter_engine \
        -genkey \
        -name secp224r1 \
        -noout \
        -out $private_key_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && cat $private_key_file

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

create_ECDSA_public_key() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local public_key_file=$3
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// create_ECDSA_public_key $2 $3" >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print
    print=`$valgrind ${OPENSSL} ec \
        $parameter_engine \
        -pubout \
        -in $private_key_file \
        -out $public_key_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && cat $public_key_file

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

hash_the_file() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local hash=$2
    local message_file=$3
    local hash_file=$4
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// hash_the_file $2 $3 $4" >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print
    print=`$valgrind ${OPENSSL} dgst \
        $parameter_engine \
        -sha256 \
        -out $hash_file \
        -binary \
        ${message_file} 2>&1`
    ret=$?
    [ $DEBUG = true ] && hexdump -x $hash_file

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

sign_the_hash() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local hash=$2
    local private_key_file=$3
    local hash_file=$4
    local signature_file=$5
    local parameter_engine
    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// sign_the_hash $2 $3 $4 $5" >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    print=`$valgrind ${OPENSSL} pkeyutl \
        $parameter_engine \
        -sign \
        -out ${signature_file} \
        -in ${hash_file} \
        -inkey ${private_key_file} \
        -keyform pem \
        -pkeyopt digest:sha256 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "$print"
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && hexdump -x $signature_file

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

validate_the_hash_with_the_signature() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local parameter_engine
    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local public_key_file=$2
    local signature_file=$3
    local hash_file=$4
    local ret
    local print

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// validate_the_hash_with_the_signature $2 $3 $4 $5" >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    print=`$valgrind ${OPENSSL} pkeyutl \
        -verify \
        $parameter_engine \
        -pubin \
        -inkey ${public_key_file} \
        -in ${hash_file} \
        -sigfile ${signature_file} \
        -keyform pem \
        -pkeyopt digest:sha256 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "return: $ret"

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> edcsa-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

cleanup() {
    [ $DEBUG = true ] && print_function
    for f in "$@"; do
        [ -f "$f" ] && rm $f 
    done
}

test_ecdsa_sign_and_verify_positive() {
    local use_engine=$1
    cleanup test.file ec_private.pem ec_public.pem ec_signature.bin test.file.hash
    create_test_file test.file 50
    create_ECDSA_private_key $use_engine ec_private.pem
    [ $? -ne 0 ] && return $FAIL
    hash_the_file $use_engine sha256 test.file test.file.hash
    [ $? -ne 0 ] && return $FAIL
    sign_the_hash $use_engine sha256 ec_private.pem test.file.hash ec_signature.bin
    [ $? -ne 0 ] && return $FAIL
    create_ECDSA_public_key $use_engine ec_private.pem ec_public.pem
    [ $? -ne 0 ] && return $FAIL
    validate_the_hash_with_the_signature $use_engine ec_public.pem ec_signature.bin test.file.hash
    [ $? -ne 0 ] && return $FAIL
    return $PASS
}

test_ecdsa_sign_and_verify_negative() {
    local use_engine=$1
    cleanup test.file ec_private.pem ec_private_2.pem ec_public_2.pem ec_signature.bin test.file.hash
    create_test_file test.file 50
    create_ECDSA_private_key $use_engine ec_private.pem
    [ $? -ne 0 ] && return $FAIL
    hash_the_file $use_engine sha256 test.file test.file.hash
    [ $? -ne 0 ] && return $FAIL
    sign_the_hash $use_engine sha256 ec_private.pem test.file.hash ec_signature.bin
    [ $? -ne 0 ] && return $FAIL
    create_ECDSA_private_key $use_engine ec_private_2.pem
    [ $? -ne 0 ] && return $FAIL
    create_ECDSA_public_key $use_engine ec_private_2.pem ec_public_2.pem
    [ $? -ne 0 ] && return $FAIL
    validate_the_hash_with_the_signature $use_engine ec_public_2.pem ec_signature.bin test.file.hash
    [ $? -eq 0 ] && return $FAIL # THIS IS A NEGATIVE TEST
    return $PASS
}

############################ main ############################

main () {
    # script arguments
    run_all=false
    run_test_number=0
    if [ "$1" -eq "$1" ] 2>/dev/null;  then
        run_test_number=$1
        echo "Run test number $test_number"
    else
        run_all=true
        echo "Run all tests"
    fi
    if [ $VALIDATE_SETUP == true ]; then
        validate_environment
        [ $? -eq 1 ] && exit 1
        print_openssl_details
        print_engine_capabilities
    fi

    if [ "$1" = "-v" ] || [ "$2" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK edcsa-sign-and-verify valgrind results - $DATESTAMP" > edcsa-sign-and-verify_$DATESTAMP.log
    fi

    if [ $run_all == true ] || [ $run_test_number -eq 1 ]; then
        test_name="ECDSA sign and verify using a matching pub key (openssl)"
        printf "Test %s: " "$test_name"
        test_ecdsa_sign_and_verify_positive false
        [ $? -eq $PASS ] && echo "PASSED" || echo "FAILED"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 2 ]; then
        test_name="ECDSA sign and verify using a matching pub key"
        printf "Test %s: " "$test_name"
        test_ecdsa_sign_and_verify_positive true
        [ $? -eq $PASS ] && echo "PASSED" || echo "FAILED"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 3 ]; then
        test_name="ECDSA sign and verify using an unmatching pub key (openssl)"
        printf "Test %s: " "$test_name"
        test_ecdsa_sign_and_verify_negative false
        [ $? -eq $PASS ] && echo "PASSED" || echo "FAILED"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 4 ]; then
        test_name="ECDSA sign and verify using an unmatching pub key (eip28)"
        printf "Test %s: " "$test_name"
        test_ecdsa_sign_and_verify_negative true
        [ $? -eq $PASS ] && echo "PASSED" || echo "FAILED"
    fi

   if [ $DO_VALGRIND ]; then
       rm $TEMPFILE
   fi

    exit 0
}

main "$@"
