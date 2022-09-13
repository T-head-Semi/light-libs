#!/bin/bash

#*****************************************************************************
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
#*****************************************************************************

############################ constants ############################

result=0
ENGINE_ID_120=eip120
KERNEL_MODULE=umpci_k
OPENSSL=$OPENSSL_DIR/apps/openssl
O1="htest_tmp1.txt" # Temporary file, used for hash eip120 output
O2="htest_tmp2.txt" # Temporary file, used for hash 'internal' output
HKEY=hkey.txt       # HMAC key file
CKEY=ckey.txt       # CMAC key file
RED='\033[0;31m'
GRN='\033[0;32m'
NC='\033[0m' # No Color
DATESTAMP=$(date +%y%m%d_%H%M)
TEMPFILE=tempfile.txt
VALGRIND_CMD='valgrind --leak-check=full --show-leak-kinds=all --log-file='$TEMPFILE

############################ functions ############################

validate_environment()
{
    if [ ! -f $OPENSSL_DIR/apps/openssl ]; then
        echo -e "${RED}Error: openssl client not found${NC}"
        exit 1
    fi
    if [ -z $OPENSSL_ENGINES ]; then
        echo -e "${RED}Error: Environment variable OPENSSL_ENGINES is undefined${NC}"
        exit 1
    fi
    if [ $(lsmod | grep "$KERNEL_MODULE" -c) -ne 1 ]; then
        echo -e "${RED}Error: kernel module $KERNEL_MODULE not found${NC}"
        exit 1
    fi

    ${OPENSSL} engine -t ${ENGINE_ID_120} &> /dev/null
    if [ $? -ne 0 ]; then
        echo -e "\n${RED}Error: OpenSSL engine ${ENGINE_ID_120} is not loaded${NC}"
        exit 1
    fi
}

check_diff()
{
    diff ${O1} ${O2} > /dev/null
    if [ $? -ne 0 ]; then
        echo $1 "(psize "$2")" FAILED
        result=1
        return
    fi
    echo $1 "(psize "$2")" PASSED
}

test_hmac()
{
    if [ $DO_VALGRIND ]; then
        echo "// $1 $2" >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | $VALGRIND ${OPENSSL} dgst -engine ${ENGINE_ID_120} -$1 -out ${O1} -mac HMAC -macopt hexkey:$hkey &> /dev/null
    if [ $? -ne 0 ]; then
        echo "openssl with " $1 "(psize "$2")" FAILED
        result=1
        return
    fi

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | ${OPENSSL} dgst -$1 -out ${O2} -mac HMAC -macopt hexkey:$hkey > /dev/null
    check_diff HMAC-$1 $2
}

test_hmacs()
{
    psizes=$2
    for psize in ${psizes[@]}; do
        test_hmac $1 $psize
    done
}

test_cmac()
{
    if [ $DO_VALGRIND ]; then
        echo "// $1 $2" >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | $VALGRIND ${OPENSSL} dgst -engine ${ENGINE_ID_120} -out ${O1} -mac CMAC -macopt cipher:$1 -macopt hexkey:$ckey &> /dev/null
    if [ $? -ne 0 ]; then
        echo "openssl with cipher " $1 "(psize "$2")" FAILED
        result=1
        return
    fi

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | ${OPENSSL} dgst -out ${O2} -mac CMAC -macopt cipher:$1 -macopt hexkey:$ckey > /dev/null
    check_diff CMAC-$1 $2
}

test_cmacs()
{
    psizes=$2
    for psize in ${psizes[@]}; do
        test_cmac $1 $psize
    done
}

test_hash()
{
    if [ $DO_VALGRIND ]; then
        echo "// $1 $2" >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | $VALGRIND ${OPENSSL} dgst -out ${O1} -$1 -engine eip120 -engine_impl &> /dev/null
    if [ $? -ne 0 ]; then
        echo "openssl with" $1 "(psize "$2")" FAILED
        result=1
        return
    fi

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> htest_$DATESTAMP.log
    fi

    head -c $2 lorum.txt | ${OPENSSL} dgst -out ${O2} -$1 &> /dev/null
    check_diff HASH-$1 $2
}

test_hashes()
{
    psizes=$2
    for psize in ${psizes[@]}; do
        test_hash $1 $psize
    done
}

main () {
    ${OPENSSL} version

    validate_environment

    if [ "$1" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK eip120 hash valgrind results - $DATESTAMP" > htest_$DATESTAMP.log
    fi

    hkey=$(<${HKEY})
    test_hmacs sha1 '0 3 56 64 128 192 256 320 1023'
    test_hmacs sha224 '0 3 56 64 128 192 256 320 1023'
    test_hmacs sha256 '0 3 56 64 128 192 256 320 1023'
    test_hmacs sha384 '0 3 56 128 256 384 512 640 1023'
    test_hmacs sha512 '0 3 56 128 256 384 512 640 1023'
    test_hmacs sha3-224 '0 3 56 144 288 432 576 720 1023'
    test_hmacs sha3-256 '0 3 56 136 272 408 544 680 1023'
    test_hmacs sha3-384 '0 3 56 104 208 312 416 520 1023'
    test_hmacs sha3-512 '0 3 56 72 144 216 288 360 1023'
    test_hmacs sm3 '0 3 56 64 128 256 384 512 640 1023'

    ckey=$(<${CKEY})
    test_cmacs aes-128-ecb '0 16 32 48 64 80 1023'
    test_cmacs sm4-ecb '0 16 32 48 64 80 1023'

    test_hashes sha1 '128 192 256 320 1023'
    test_hashes sha224 '128 192 256 320 1023'
    test_hashes sha256 '128 192 256 320 1023'
    test_hashes sha384 '256 384 512 640 1023'
    test_hashes sha512 '256 384 512 640 1023'
    test_hashes sha3-224 '288 432 576 720 1023'
    test_hashes sha3-256 '272 408 544 680 1023'
    test_hashes sha3-384 '208 312 416 520 1023'
    test_hashes sha3-512 '144 216 288 360 1023'
    test_hashes sm3 '128 256 384 512 640 1023'

    if [ $DO_VALGRIND ]; then
        rm $TEMPFILE
    fi

    unset DO_VALGRIND
    unset VALGRIND

    test_hashes sha1 '3 56 64'
    test_hashes sha224 '3 56 64'
    test_hashes sha256 '3 56 64'
    test_hashes sha384 '3 56 128'
    test_hashes sha512 '3 56 128'
    test_hashes sha3-224 '3 56 144'
    test_hashes sha3-256 '3 56 136'
    test_hashes sha3-384 '3 56 104'
    test_hashes sha3-512 '3 56 72'
    test_hashes sm3 '3 56 64'

    rm ${O1} ${O2}
    if [ ${result} -ne 0 ]; then
        echo -e "\n${RED}Hash tests FAILED${NC}"
        return
    fi
    echo -e "\n${GRN}Hash tests PASSED${NC}"
}

main "$@"
