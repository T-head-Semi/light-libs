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
#     1. SM2 sign (openssl) and verify (openssl) / matching private and public key
#     2. SM2 sign (engine) and verify (engine) / matching private and public key
#     3. SM2 sign (engine) and verify (openssl) / matching private and public key
#     4. SM2 sign (openssl) and verify (engine) / matching private and public key
#     5. SM2 sign (openssl) and verify (openssl) / unmatching private and public key
#     6. SM2 sign (engine) and verify (engine) / unmatching private and public key
#
# Setup:
# 1. Configure the openssl:     export OPENSSL_DIR="/path/to/openssl-1.1.1k"
# 2. Set engine path:           export OPENSSL_ENGINES=/path/to/engines
# 3. Configure the SM2 tool:    export SM2TOOL_DIR="/path/to/sm2"
#
# Usage:
#    run all tests:    bash sm2-sign-and-verify.sh
#    run a test:       bash sm2-sign-and-verify.sh <test number>
#
# Date:        23/7/2021 
##############################################################################

############################ constants ############################
PASS=0
FAIL=1
DEBUG=false
VALIDATE_SETUP=true
ENGINE_ID_28=eip28pka
KERNEL_MODULE=umpci_k
OPENSSL=$OPENSSL_DIR/apps/openssl
SM2SIGN=$SM2TOOL_DIR/sm2sign
SM2VERIFY=$SM2TOOL_DIR/sm2verify
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
    if [ -z $SM2TOOL_DIR ]; then
        echo "Error: Environment variable SM2TOOL_DIR is undefined"
        return $FAIL
    fi
    if [ ! -f $SM2SIGN ]; then
        echo "Error: sm2sign not found"
        return $FAIL
    fi
    if [ ! -f $SM2VERIFY ]; then
        echo "Error: sm2verify not found"
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

print_configuration() {
    echo "DEBUG           $DEBUG         "
    echo "VALIDATE_SETUP  $VALIDATE_SETUP"
    echo "ENGINE_ID_28    $ENGINE_ID_28  "
    echo "KERNEL_MODULE   $KERNEL_MODULE "
    echo "OPENSSL         $OPENSSL       "
    echo "SM2SIGN         $SM2SIGN       "
    echo "SM2VERIFY       $SM2VERIFY     "
}

create_test_file() {
    local filename=$1
    local length=$2
    [ $DEBUG = true ] && print_function
    echo $(${OPENSSL} rand -base64 $length) > $filename
    [ $DEBUG = true ] && cat $filename
}

create_SM2_private_key() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local private_key_file=$2
    local parameter_engine
    [ $engine == true ] && parameter_engine=-engine=$ENGINE_ID_28
    local ret
    local print
    # generate private SM2 key:
    # apps/openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:sm2 -out private.pem
    # examine private SM2 key:
    # openssl ec -in private.pem -noout -text
    print=`$valgrind ${OPENSSL} genpkey \
        $parameter_engine \
        -algorithm EC \
        -pkeyopt ec_paramgen_curve:sm2 \
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
    # generate public SM2 key:
    # openssl ec -pubout -in private.pem -out public.pem
    # examine public SM2 key:
    # openssl ec -noout -text -inform PEM -in public.pem -pubin
    print=`${OPENSSL} ec \
        $parameter_engine \
        -pubout \
        -in $private_key_file \
        -out $public_key_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "return: $ret"
    [ $DEBUG = true ] && cat $public_key_file
    return $ret
}

hash_the_file_and_sign_the_hash() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local message_file=$2
    local private_key_file=$3
    local sm2id=$4
    local signature_file=$5
    local ret
    local print
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// hash_the_file_and_sign_the_hash $2 $3 $4 $5" >> sm2-sign-and-verify_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=--engine=$ENGINE_ID_28
    print=`$valgrind $SM2SIGN \
        $parameter_engine \
        --message $message_file \
        --key $private_key_file \
        --id $sm2id \
        --signature $signature_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "*********************** START *******************************"
    [ $DEBUG = true ] && echo "$print"
    [ $DEBUG = true ] && echo "*********************** END *******************************"
    [ $DEBUG = true ] && echo "return: $ret"

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> sm2-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

hash_the_file_and_validate_the_signature() {
    [ $DEBUG = true ] && print_function
    local engine=$1
    local message_file=$2
    local public_key_file=$3
    local sm2id=$4
    local signature_file=$5
    local ret
    local print
    local parameter_engine

    if [ $engine == true ]; then
        local valgrind=$VALGRIND
        local dovalgrind=$DO_VALGRIND
    fi

    if [ $dovalgrind ]; then
        echo "// hash_the_file_and_validate_the_signature $2 $3 $4 $5" >> sm2-sign-and-verify_$DATESTAMP.log
    fi

    [ $engine == true ] && parameter_engine=--engine=$ENGINE_ID_28
    print=`$valgrind $SM2VERIFY \
        $parameter_engine \
        --message $message_file \
        --key $public_key_file \
        --id $sm2id \
        --signature $signature_file 2>&1`
    ret=$?
    [ $DEBUG = true ] && echo "*********************** START *******************************"
    [ $DEBUG = true ] && echo "$print"
    [ $DEBUG = true ] && echo "*********************** END *******************************"
    [ $DEBUG = true ] && echo "return: $ret"

    if [ $dovalgrind ]; then
        cat $TEMPFILE >> sm2-sign-and-verify_$DATESTAMP.log
    fi

    return $ret
}

cleanup() {
    [ $DEBUG = true ] && print_function
    for f in "$@"; do
        [ -f "$f" ] && rm $f 
    done
}

test_sm2_sign_and_verify_positive() {
    local use_engine_sign=$1
    local use_engine_verify=$2
    cleanup test.file private.pem public.pem signature.bin test.file.hash
    create_test_file test.file 50
    create_SM2_private_key false private.pem
    [ $? -ne 0 ] && return 1
    hash_the_file_and_sign_the_hash $use_engine_sign test.file private.pem 1234letter signature.bin
    [ $? -ne 0 ] && return 2
    derive_public_key false private.pem public.pem
    [ $? -ne 0 ] && return 3
    hash_the_file_and_validate_the_signature $use_engine_verify test.file public.pem 1234letter signature.bin
    [ $? -ne 0 ] && return 4
    return $PASS
}

test_sm2_sign_and_verify_negative() {
    local use_engine_sign=$1
    local use_engine_verify=$2
    cleanup test.file private.pem private_2.pem public_2.pem signature.bin test.file.hash
    create_test_file test.file 50
    create_SM2_private_key false private.pem
    [ $? -ne 0 ] && return 1
    hash_the_file_and_sign_the_hash $use_engine_sign test.file private.pem 1234letter signature.bin
    [ $? -ne 0 ] && return 2
    create_SM2_private_key false private_2.pem
    [ $? -ne 0 ] && return 3
    derive_public_key false private_2.pem public_2.pem
    [ $? -ne 0 ] && return 4
    hash_the_file_and_validate_the_signature $use_engine_verify test.file public_2.pem 1234letter signature.bin
    [ $? -eq 0 ] && return 5 # THIS IS A NEGATIVE TEST
    return $PASS
}

############################ main ############################

main () {
    echo "Test: SM2 sign and verify"
    # arguments
    run_all=false
    run_test_number=0
    if [ "$1" -eq "$1" ] 2>/dev/null;  then
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
        print_configuration
        print_openssl_details
        print_engine_capabilities
    fi

    if [ "$1" = "-v" ] || [ "$2" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK sm2-sign-and-verify valgrind results - $DATESTAMP" > sm2-sign-and-verify_$DATESTAMP.log
    fi

    # tests:
    if [ $run_all == true ] || [ $run_test_number -eq 1 ]; then
        test_name="SM2 sign and verify using a matching pub key (openssl, openssl)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_positive false false
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 2 ]; then
        test_name="SM2 sign and verify using a matching pub key (engine, engine)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_positive true true
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 3 ]; then
        test_name="SM2 sign and verify using a matching pub key (engine, openssl)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_positive true false
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 4 ]; then
        test_name="SM2 sign and verify using a matching pub key (openssl, engine)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_positive false true
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 5 ]; then
        test_name="SM2 sign and verify using an unmatching pub key (openssl, openssl)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_negative false false
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi
    if [ $run_all == true ] || [ $run_test_number -eq 6 ]; then
        test_name="SM2 sign and verify using an unmatching pub key (engine, engine)"
        printf "Test %s: " "$test_name"
        test_sm2_sign_and_verify_negative true true
        result=$?
        [ $result -eq $PASS ] && echo "PASSED" || echo "FAILED ($result)"
    fi

    if [ $DO_VALGRIND ]; then
        rm $TEMPFILE
    fi

    exit 0
}

main "$@"
