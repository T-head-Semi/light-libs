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
#
# Tests:
#   1. key agreement, curve X25519, matching private key public peer key (openssl, openssl)
#   2. key agreement, curve X25519, matching private key public peer key (engine, engine)
#   3. Key agreement, curve X25519, unmatching private key public peer key (openssl, engine)
#
# Setup:
# 1. Configure the openssl:   export OPENSSL_DIR="/path/to/openssl-1.1.1k"
# 2. Set engine path:         export OPENSSL_ENGINES=/path/to/engines
# 
# Usage:
#    run all tests:    bash x25519-key-agreement.sh
#    run a test:       bash x25519-key-agreement.sh <test number>
#
# Date:        12/8/2021
##############################################################################

############################ constants ############################
PASS=0
FAIL=1
DEBUG=false
VALIDATE_SETUP=true
ENGINE_ID_28=eip28pka
KERNEL_MODULE=umpci_k
OPENSSL=$OPENSSL_DIR/apps/openssl
DATESTAMP=$(date +%y%m%d_%H%M)
TEMPFILE=tempfile.txt
VALGRIND_CMD='valgrind --leak-check=full --show-leak-kinds=all --log-file='$TEMPFILE

############################ functions ############################
print_function() {
    echo "${FUNCNAME[1]}"
}

print_openssl_details() {
    [ $DEBUG = true ] && print_function
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

is_engine_available() {
    if [ $(${OPENSSL} engine -t eip28pka | grep -c "\[ available \]") -ne 1 ]; then
        echo "Error: engine unavailable"
        return $FAIL
    fi
    return $PASS
}

print_configuration() {
    echo "DEBUG           $DEBUG         "
    echo "VALIDATE_SETUP  $VALIDATE_SETUP"
    echo "ENGINE_ID_28    $ENGINE_ID_28  "
    echo "KERNEL_MODULE   $KERNEL_MODULE "
    echo "OPENSSL         $OPENSSL       "
}

create_test_file() {
    local filename=$1
    local length=$2
    [ $DEBUG = true ] && print_function
    echo $(${OPENSSL} rand -base64 $length) > $filename
    [ $DEBUG = true ] && cat $filename
}

create_X25519_private_key() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local parameter_engine
    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print
    # generate private key:
    # openssl genpkey -algorithm X25519 -out private.pem
    # examine private key:
    # openssl ec -in private.pem -noout -text
    print=`${OPENSSL} genpkey \
        $parameter_engine \
        -algorithm X25519 \
        -out $private_key_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "$print"
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && cat $private_key_file
    return $ret
}

derive_public_key() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local public_key_file=$3
    local parameter_engine
    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print
    # generate public key:
    # openssl ec -pubout -in private.pem -out public.pem
    # examine public key:
    # openssl pkey -noout -text -inform PEM -in public.pem -pubin
    print=`${OPENSSL} pkey \
        $parameter_engine \
        -pubout \
        -in $private_key_file \
        -out $public_key_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && cat $public_key_file
    return $ret
}

derive_shared_secret() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local public_key_peer_file=$3
    local output_secert_file=$4
    local ret
    local print
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// derive_shared_secret $2 $3 $4" >> x25519-key-agreement_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    # openssl pkeyutl -derive -inkey alice.pem -peerkey bob.pub -out alicebob.key
    print=`$valgrind $OPENSSL pkeyutl \
        -derive \
        $parameter_engine \
        -inkey $private_key_file \
        -peerkey $public_key_peer_file \
        -out $output_secert_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "*********************** START *******************************"
    [ $DEBUG = true ] && echo "$print"
    [ $DEBUG = true ] && echo "*********************** END *******************************"
    [ $DEBUG = true ] && echo "return: $ret"

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> x25519-key-agreement_$DATESTAMP.log
    fi

    return $ret
}

cleanup() {
    [ $DEBUG = true ] && print_function
    for f in "$@"; do
        [ -f "$f" ] && rm $f 
    done
}

test_key_exchange_positive () {
    local use_engine_1=$1
    local use_engine_2=$2
    cleanup private1.pem public1.pem private2.pem public2.pem secret1.bin secret2.bin 
    create_X25519_private_key false private1.pem
    [ $? -ne 0 ] && return 1
    derive_public_key false private1.pem public1.pem
    [ $? -ne 0 ] && return 2
    create_X25519_private_key false private2.pem
    [ $? -ne 0 ] && return 3
    derive_public_key false private2.pem public2.pem
    [ $? -ne 0 ] && return 4
    derive_shared_secret $use_engine_1 private1.pem public2.pem secret1.bin
    [ $? -ne 0 ] && return 5
    derive_shared_secret $use_engine_2 private2.pem public1.pem secret2.bin
    [ $? -ne 0 ] && return 6
    cmp -s secret1.bin secret2.bin
    [ $? -ne 0 ] && return 7
    return $PASS
}

test_key_exchange_negative() {
    local use_engine_1=$1
    local use_engine_2=$2
    cleanup private1.pem public1.pem private2.pem public2.pem secret1.bin secret2.bin 
    create_X25519_private_key false private1.pem
    [ $? -ne 0 ] && return 1
    derive_public_key false private1.pem public1.pem
    [ $? -ne 0 ] && return 2
    create_X25519_private_key false private2.pem
    [ $? -ne 0 ] && return 3
    derive_public_key false private2.pem public2.pem
    [ $? -ne 0 ] && return 4
    create_X25519_private_key false $curve private3.pem
    [ $? -ne 0 ] && return 5
    derive_public_key false private3.pem public3.pem
    [ $? -ne 0 ] && return 6
    derive_shared_secret $use_engine_1 private1.pem public2.pem secret1.bin
    [ $? -ne 0 ] && return 7
    derive_shared_secret $use_engine_2 private2.pem public3.pem secret2.bin
    [ $? -ne 0 ] && return 8
    cmp -s secret1.bin secret2.bin
    [ $? -eq 0 ] && return 6
    return $PASS
}

############################ main ############################

main () {
    echo "Test: x25519 key agreement"
    # arguments
    run_all=false
    run_test_number=0
    if [ "$1" -eq "$1" ] 2>/dev/null; then
        run_test_number=$1
        echo "Run test number $test_number"
    else
        run_all=true
        echo "Run all tests"
    fi
    # validation
    if [ $VALIDATE_SETUP == true ]; then
        echo "validate setup"
        validate_environment
        [ $? -eq $FAIL ] && exit 1
        is_engine_available
        [ $? -eq $FAIL ] && exit 1
        print_configuration
        print_openssl_details
        print_engine_capabilities
    fi

    if [ "$1" = "-v" ] || [ "$2" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK x25519-key-agreement valgrind results - $DATESTAMP" > x25519-key-agreement_$DATESTAMP.log
    fi

    # tests:
    tests_run=0
    tests_pass=0
    tests_total=3
    if [ $run_all == true ] || [ $run_test_number -eq 1 ]; then
        test_name="Key agreement, curve X25519, matching private key public peer key (openssl, openssl)"
        printf "Test %s: " "$test_name"
        test_key_exchange_positive false false X25519
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
        if [ $result -eq $PASS ]; then (( tests_pass++ )); fi
        (( tests_run++ ))
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 2 ]; then
        test_name="Key agreement, curve X25519, matching private key public peer key (openssl, engine)"
        printf "Test %s: " "$test_name"
        test_key_exchange_positive false true X25519
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
        if [ $result -eq $PASS ]; then (( tests_pass++ )); fi
        (( tests_run++ ))
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 3 ]; then
        test_name="Key agreement, curve X25519, unmatching private key public peer key (openssl, engine)"
        printf "Test %s: " "$test_name"
        test_key_exchange_negative false true X25519
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
        if [ $result -eq $PASS ]; then (( tests_pass++ )); fi
        (( tests_run++ ))
    fi
    echo "tests: $tests_total run: $tests_run passed: $tests_pass"

    if [ $DO_VALGRIND ]; then
        rm $TEMPFILE
    fi

    exit 0
}

main "$@"
