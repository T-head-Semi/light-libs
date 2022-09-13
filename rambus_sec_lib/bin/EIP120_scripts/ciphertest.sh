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

############################ constants ############################

result=0
ENGINE_ID_120=eip120
KERNEL_MODULE=umpci_k
OPENSSL=$OPENSSL_DIR/apps/openssl
O1="cipherout1.txt" # Temporary file, used for cipher eip120 output
O2="cipherout2.txt" # Temporary file, used for cipher 'internal' output
#MES=testdatain.txt  #Data to encrypt or Decrypt
MES=message.txt
BIGMES=testdatain.txt
IVFILE=iv.txt                            # IV file for test application
KEYFILE=key.txt                          # Key file for test application
TAGFILE=tag.txt                          # tag file for test application
AADFILE=aad.txt                          # aad file for test application
RED='\033[0;31m'
GRN='\033[0;32m'
NC='\033[0m' # No Color
testarray=(aes-128-ecb aes-192-ecb aes-256-ecb
           aes-128-cbc aes-192-cbc aes-256-cbc
           aes-128-ctr aes-192-ctr aes-256-ctr
           des-ecb des-cbc des-cfb des-ofb
           des-ede3-ecb des-ede3-cbc des-ede3-cfb des-ede3-ofb
           sm4-ecb sm4-cbc sm4-ctr)
keysizearray=(16 24 32
              16 24 32
              16 24 32
              8 8 8 8
              24 24 24 24
              16 16 16)
ivsizearray=(0 0 0
             16 16 16
             16 16 16
             0 8 8 8
             0 8 8 8
             0 16 16)
testarrayxts=(aes-128-xts aes-256-xts)
testarraygcm=(aes-128-gcm aes-192-gcm aes-256-gcm)
DATESTAMP=$(date +%y%m%d_%H%M)
TEMPFILE=tempfile.txt
VALGRIND_CMD='valgrind --leak-check=full --show-leak-kinds=all --log-file='$TEMPFILE

############################ functions ############################

validate_environment()
{
    if [ ! -f $OPENSSL ]; then
        echo "Error: openssl client not found"
        exit 1
    fi
    if [ -z $OPENSSL_ENGINES ]; then
        echo "Error: Environment variable OPENSSL_ENGINES is undefined"
        exit 1
    fi
    if [ $(lsmod | grep "$KERNEL_MODULE" -c) -ne 1 ]; then
        echo "Error: kernel module $KERNEL_MODULE not found"
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
        echo -e "\n${RED}$1 $2 tests FAILED${NC}"
        result=1
        return
    fi
    if [ $2 = e ]; then
        echo $1 ENCRYPTION PASSED
    else
        echo $1 DECRYPTION PASSED
    fi
}

test_aes()
{
    if [ $DO_VALGRIND ]; then
        echo "// test_aes $1 $2" >> ciphertest_$DATESTAMP.log
    fi
    $VALGRIND ./../build/cipher --type $1 --engine ${ENGINE_ID_120} --in ${BIGMES} --out ${O1} --op $2 --iv ${IVFILE} --key ${KEYFILE} > /dev/null
    if [ $1 = aes-128-ecb ] || [ $1 = aes-192-ecb ] || [ $1 = aes-256-ecb ] ||
       [ $1 = des-ecb ] || [ $1 = des-ede3-ecb ] || [ $1 = sm4-ecb ]; then
    ${OPENSSL} $1 -nosalt -in ${BIGMES} -out ${O2} -$2 -K ${ckey:0:$3} -nopad > /dev/null
    else
    ${OPENSSL} $1 -nosalt -in ${BIGMES} -out ${O2} -$2 -K ${ckey:0:$3} -iv ${ivkey:0:$4} -nopad > /dev/null
    fi
    check_diff $1 $2
    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> ciphertest_$DATESTAMP.log
    fi
}

test_xts()
{
    if [ $DO_VALGRIND ]; then
        echo "// test_xts $1 $2" >> ciphertest_$DATESTAMP.log
    fi

    $VALGRIND ./../build/cipher --type $1 --engine ${ENGINE_ID_120} --in ${MES} --out ${O1} --outdef ${O2} --op $2 --iv ${IVFILE} --key ${KEYFILE} > /dev/null
    check_diff $1 $2

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> ciphertest_$DATESTAMP.log
    fi
}

test_gcm()
{
    if [ $DO_VALGRIND ]; then
        echo "// test_gcm $1 $2" >> ciphertest_$DATESTAMP.log
    fi

    if [ $2 = e ]; then
        $VALGRIND ./../build/cipher --type $1 --engine ${ENGINE_ID_120} --in ${MES} --out ${O1} --outdef ${O2} --op $2 --iv ${IVFILE} --key ${KEYFILE} --tag ${TAGFILE} --aad ${AADFILE} > /dev/null
        check_diff $1 $2
    else
        $VALGRIND ./../build/cipher --type $1 --engine ${ENGINE_ID_120} --in ${O1} --out ${O1} --outdef ${O2} --op $2 --iv ${IVFILE} --key ${KEYFILE} --tag ${TAGFILE} --aad ${AADFILE} > /dev/null
        check_diff $1 $2
    fi

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> ciphertest_$DATESTAMP.log
    fi
}

main () {
    ${OPENSSL} version

    validate_environment

    if [ "$1" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK ciphertest valgrind results - $DATESTAMP" > ciphertest_$DATESTAMP.log
    fi

    ckey=$(<${KEYFILE})
    ivkey=$(<${IVFILE})
    for ((loop = 0; loop < ${#testarray[@]}; loop++)); do
        test_aes ${testarray[loop]} e ${keysizearray[loop]}*2 ${ivsizearray[loop]}*2
        test_aes ${testarray[loop]} d ${keysizearray[loop]}*2 ${ivsizearray[loop]}*2
    done
    for loop in ${testarraygcm[@]}; do
        test_gcm $loop e
        test_gcm $loop d
    done
    for loop in ${testarrayxts[@]}; do
        test_xts $loop e
        test_xts $loop d
    done

    rm ${O1} ${O2}
    if [ ${result} -ne 0 ]; then
        echo -e "\n${RED}Cipher tests FAILED${NC}"
        return
    fi
    echo -e "\n${GRN}Cipher tests PASSED${NC}"

    if [ $DO_VALGRIND ]; then
        rm $TEMPFILE
    fi
}

main "$@"
