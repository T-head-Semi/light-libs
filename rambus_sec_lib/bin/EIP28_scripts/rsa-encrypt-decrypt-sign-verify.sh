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
DATESTAMP=$(date +%y%m%d_%H%M)
TEMPFILE=tempfile.txt
VALGRIND_CMD='valgrind --leak-check=full --show-leak-kinds=all --log-file='$TEMPFILE
# configure based engine id
engine_id=eip28pka

############################ functions ############################

if [ -z "${OPENSSL_DIR}" ]; then
    echo "Error: OPENSSL_DIR not set."
    echo "Please let an environment named OPENSSL_DIR refer to the"
    echo "directory where the OpenSSL package was built, such that"
    echo "\${OPENSSL_DIR}/apps/openssl refers to the openssl app to use."
    exit 1
fi

# Run various test with the following instance of OpenSSL.
OSSL=${OPENSSL_DIR}/apps/openssl
if [ ! -x ${OSSL} ]; then
    echo "Error: ${OSSL} not found or not executable?!"
    exit 1
fi

# Pad `file` such that its size equals `target_size`.
padd_file () {
    local file=$1
    local target_size=$2
    local padding="++++++++"
    local text=$(cat ${file})

    while (( ${#padding} < ${target_size} )); do
        padding="${padding}${padding}"
    done
    echo "${padding}${text}" | tail --bytes ${target_size} - > ${file}
}

# Generate an RSA keypair of the specified size and store
# it in the specified .pem file.
ossl_gen_rsa_key () {
    local nbits=$1
    local keyfile=$2
    local keyform=${keyfile##*.}

    if [[ "${keyform}" != "pem" ]]; then
        echo "ossl_gen_rsa_key: filename must end with .pem"
    else
        ${OSSL} genpkey -out ${keyfile} -outform ${keyform} \
                -algorithm RSA -pkeyopt rsa_keygen_bits:${nbits}
    fi
}

# Show the RSA keypair stored in the specified .pem or .der file.
ossl_show_rsa_key () {
    local keyfile=$1
    local keyform=${keyfile##*.}

    if [[ "${keyform}" != "pem" && "${keyform}" != "der" ]]; then
        echo "ossl_show_key: filename must end with .der or .pem"
    else
        if [[ "${keyform}" == "pem" ]]; then
            # Convert .pem to .der
            ${OSSL} rsa -out __@der@__ -outform der -in ${keyfile}
            ${OSSL} asn1parse -in __@der@__ -inform der
            rm -f __@der@__
        else
            ${OSSL} asn1parse -in ${keyfile} -inform ${keyform}
        fi
    fi
}

# Use the indicated RSA scheme (pkcs1 or pss) to sign the given
# `msgfile`, with the (private) RSA key coming from `keyfile` and
# store the signature in `sigfile`.
# For the moment, the hash algorithm used is always SHA256.
ossl_rsa_sign () {
    local msgfile=$1
    local keyfile=$2
    local sigfile=$3
    local padding=$4 # pkcs1 or pss
    local keyform=${keyfile##*.}
    local opt_saltlen=""
    local hashalg="sha256"

    if [[ "${padding}" == "pss" ]]; then
        opt_saltlen="-pkeyopt rsa_pss_saltlen:32"
    fi

    ${OSSL} dgst -${hashalg} -out __@hash@__ -binary ${msgfile}
    ${OSSL} pkeyutl -sign -out ${sigfile} -in __@hash@__ \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} \
            -pkeyopt digest:${hashalg} ${opt_saltlen}
    rm -f __@hash@__
}

# Use the indicated RSA scheme (pkcs1 or pss) to verify if the
# signature in `sigfile` matches the given `msgfile`, with the
# (public) RSA key coming from `keyfile`.
# For the moment, the hash algorithm used is always SHA256.
ossl_rsa_vrfy() {
    local msgfile=$1
    local keyfile=$2
    local sigfile=$3
    local verfile=$4
    local padding=$5
    local keyform=${keyfile##*.}
    local opt_saltlen=""
    local hashalg="sha256"

    if [[ "${padding}" == "pss" ]]; then
        opt_saltlen="-pkeyopt rsa_pss_saltlen:32"
    fi

    ${OSSL} dgst -${hashalg} -out __@hash@__ -binary ${msgfile}
    ${OSSL} pkeyutl -verify -in __@hash@__ -sigfile ${sigfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} \
            -pkeyopt digest:${hashalg} ${opt_saltlen} \
            -out ${verfile}
    rm -f __@hash@__
}

# Use the indicated RSA scheme (pkcs1 or pss) to sign the given
# `msgfile`, with the (private) RSA key coming from `keyfile` and
# store the signature in `sigfile`.
# For the moment, the hash algorithm used is always SHA256.
ossl_engine_rsa_sign () {
    local msgfile=$1
    local keyfile=$2
    local sigfile=$3
    local padding=$4 # pkcs1 or pss
    local keyform=${keyfile##*.}
    local opt_saltlen=""
    local hashalg="sha256"

    if [[ "${padding}" == "pss" ]]; then
        opt_saltlen="-pkeyopt rsa_pss_saltlen:32"
    fi

    if [ $DO_VALGRIND ]; then
        echo "// ossl_engine_rsa_sign $2 $3 $4" >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    ${OSSL} dgst -${hashalg} -out __@hash@__ -binary ${msgfile}
    $VALGRIND ${OSSL} pkeyutl -engine ${engine_id} -sign -out ${sigfile} -in __@hash@__ \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} \
            -pkeyopt digest:${hashalg} ${opt_saltlen}

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    rm -f __@hash@__
}

# Use the indicated RSA scheme (pkcs1 or pss) to verify if the
# signature in `sigfile` matches the given `msgfile`, with the
# (public) RSA key coming from `keyfile`.
# For the moment, the hash algorithm used is always SHA256.
ossl_engine_rsa_vrfy() {
    local msgfile=$1
    local keyfile=$2
    local sigfile=$3
    local verfile=$4
    local padding=$5
    local keyform=${keyfile##*.}
    local opt_saltlen=""
    local hashalg="sha256"

    if [[ "${padding}" == "pss" ]]; then
        opt_saltlen="-pkeyopt rsa_pss_saltlen:32"
    fi

    if [ $DO_VALGRIND ]; then
        echo "// ossl_engine_rsa_vrfy $2 $3 $4 $5" >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    ${OSSL} dgst -${hashalg} -out __@hash@__ -binary ${msgfile}
    $VALGRIND ${OSSL} pkeyutl -engine ${engine_id} -verify -in __@hash@__ -sigfile ${sigfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} \
            -pkeyopt digest:${hashalg} ${opt_saltlen} \
            -out ${verfile}

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    rm -f __@hash@__
}

# Use the indicated RSA scheme (none, pkcs1 or oaep) to encrypt the given
# `ptxfile`, with the (public) RSA key coming from `keyfile` and
# store the cryptogram in `ctxfile`.
ossl_rsa_encrypt () {
    local ptxfile=$1
    local keyfile=$2
    local ctxfile=$3
    local padding=$4 # none, pkcs1 or oaep
    local hashalg="sha1"
    local keyform=${keyfile##*.}
    local opt_mgf1_md=""
    local opt_digest=""

    if [[ "${padding}" == "oaep" ]]; then
        :
        # opt_mgf1_md="-pkeyopt rsa_mgf1_md:${hashalg}"
        # opt_digest="-pkeyopt digest:${hashalg}"
    fi

    ${OSSL} pkeyutl -encrypt -out ${ctxfile} -in ${ptxfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} ${opt_mgf1_md} ${opt_digest}
}

# Use the indicated RSA scheme (none, pkcs1 or oaep) to encrypt the given
# `ptxfile`, with the (public) RSA key coming from `keyfile` and
# store the cryptogram in `ctxfile`.
ossl_engine_rsa_encrypt () {
    local ptxfile=$1
    local keyfile=$2
    local ctxfile=$3
    local padding=$4 # none, pkcs1 or oaep
    local hashalg="sha1"
    local keyform=${keyfile##*.}
    local opt_mgf1_md=""
    local opt_digest=""

    if [[ "${padding}" == "oaep" ]]; then
        :
        # opt_mgf1_md="-pkeyopt rsa_mgf1_md:${hashalg}"
        # opt_digest="-pkeyopt digest:${hashalg}"
    fi

    if [ $DO_VALGRIND ]; then
        echo "// ossl_engine_rsa_encrypt $2 $3 $4" >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    $VALGRIND ${OSSL} pkeyutl -engine ${engine_id} -encrypt -out ${ctxfile} -in ${ptxfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} ${opt_mgf1_md} ${opt_digest}

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi
}

# Use the indicated RSA scheme (none, pkcs1 or oaep) to decrypt the given
# `ctxfile`, with the (private) RSA key coming from `keyfile` and
# store the plain text in `msgfile`.
ossl_rsa_decrypt () {
    local ctxfile=$1
    local keyfile=$2
    local msgfile=$3
    local padding=$4 # none, pkcs1 or oaep
    local hashalg="sha1"
    local keyform=${keyfile##*.}
    local opt_mgf1_md=""
    local opt_digest=""

    if [[ "${padding}" == "oaep" ]]; then
        :
        # opt_mgf1_md="-pkeyopt rsa_mgf1_md:${hashalg}"
        # opt_digest="-pkeyopt digest:${hashalg}"
    fi

    ${OSSL} pkeyutl -decrypt -out ${msgfile} -in ${ctxfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} ${opt_mgf1_md} ${opt_digest}
}

# Use the indicated RSA scheme (none, pkcs1 or oaep) to decrypt the given
# `ctxfile`, with the (private) RSA key coming from `keyfile` and
# store the plain text in `msgfile`.
ossl_engine_rsa_decrypt () {
    local ctxfile=$1
    local keyfile=$2
    local msgfile=$3
    local padding=$4 # none, pkcs1 or oaep
    local hashalg="sha1"
    local keyform=${keyfile##*.}
    local opt_mgf1_md=""
    local opt_digest=""

    if [[ "${padding}" == "oaep" ]]; then
        :
        # opt_mgf1_md="-pkeyopt rsa_mgf1_md:${hashalg}"
        # opt_digest="-pkeyopt digest:${hashalg}"
    fi

    if [ $DO_VALGRIND ]; then
        echo "// ossl_engine_rsa_decrypt $2 $3 $4" >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    $VALGRIND ${OSSL} pkeyutl -engine ${engine_id} -decrypt -out ${msgfile} -in ${ctxfile} \
            -inkey ${keyfile} -keyform ${keyform} \
            -pkeyopt rsa_padding_mode:${padding} ${opt_mgf1_md} ${opt_digest}

    if [ $DO_VALGRIND ]; then
        cat $TEMPFILE >> rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi
}

# Perform an RSA sign-and-verify test with the specified
# RSA key size and padding/scheme (i.e. pkcs1 or pss).
rsa_sign_and_vrfy_test() {
    local nbits=$1
    local padding=$2
    local keyfile="rsakey_${nbits}.pem"
    local msgfile="message.txt"
    local sigfile="signature_${padding}_${nbits}"
    local verfile="verify_${padding}_${nbits}"
    local res="FAILED"

    # ossl_gen_rsa_key ${nbits} ${keyfile}
    # ossl_show_rsa_key ${keyfile}
    echo "My message to sign." > ${msgfile}
    # test engine sign
    ossl_engine_rsa_sign ${msgfile} ${keyfile} ${sigfile} ${padding}
    ossl_rsa_vrfy ${msgfile} ${keyfile} ${sigfile} ${verfile} ${padding}
    if grep -q 'Successfully' ${verfile}; then
        res="PASSED"
    else
        res="FAILED"
    fi
    echo "[INFO] ${engine_id} rsa_sign_test (${nbits}, ${padding}):${res}"
    # test engine verify
    ossl_rsa_sign ${msgfile} ${keyfile} ${sigfile} ${padding}
    ossl_engine_rsa_vrfy ${msgfile} ${keyfile} ${sigfile} ${verfile} ${padding}
    if grep -q 'Successfully' ${verfile}; then
        res="PASSED"
    else
        res="FAILED"
    fi
    echo "[INFO] ${engine_id} rsa_verify_test (${nbits}, ${padding}):${res}"
}

# Perform an RSA encrypt-and-decrypt test with the specified
# RSA key size and padding/scheme (i.e. none, pkcs1 or oaep).
rsa_encrypt_and_decrypt_test() {
    local nbits=$1
    local padding=$2
    local keyfile="rsakey_${nbits}.pem"
    local ptxfile="plaintxt"
    local ctxfile="ciphertxt_${padding}_${nbits}"
    local outfile="plain.out"
    local res="FAILED"

    echo "My message to encrypt." > ${ptxfile}
    if [[ "${padding}" == "none" ]]; then
        # Apparently, with padding set to `none`,
        # the size of the input must equal the modulus size?!
        cp ${ptxfile} ${ptxfile}_${nbits}_none
        ptxfile=${ptxfile}_${nbits}_none
        padd_file ${ptxfile} $(( ${nbits} / 8 ))
    fi
    ossl_engine_rsa_encrypt ${ptxfile} ${keyfile} ${ctxfile} ${padding}
    ossl_rsa_decrypt ${ctxfile} ${keyfile} ${outfile} ${padding}
    /usr/bin/diff --brief ${ptxfile} ${outfile} && res="PASSED"
    echo "[INFO] ${engine_id} rsa_encrypt_test (${nbits}, ${padding}): ${res}"

    res="FAILED"
    ossl_rsa_encrypt ${ptxfile} ${keyfile} ${ctxfile} ${padding}
    ossl_engine_rsa_decrypt ${ctxfile} ${keyfile} ${outfile} ${padding}
    /usr/bin/diff --brief ${ptxfile} ${outfile} && res="PASSED"
    echo "[INFO] ${engine_id} rsa_decrypt_test (${nbits}, ${padding}): ${res}"
}

main () {
    ${OSSL} version


    if [ "$1" = "-v" ]; then
        DO_VALGRIND=1
        VALGRIND=$VALGRIND_CMD
        echo "// OS_IK rsa-encrypt-decrypt-sign-verify valgrind results - $DATESTAMP" > rsa-encrypt-decrypt-sign-verify_$DATESTAMP.log
    fi

    echo "**************************"
    echo "Generate RSA Keys"
    ossl_gen_rsa_key 2048 rsakey_2048.pem
    ossl_gen_rsa_key 3072 rsakey_3072.pem
    echo "**************************"
    echo "Test RSA Sign & Verify"
    rsa_sign_and_vrfy_test 2048 pkcs1
    rsa_sign_and_vrfy_test 3072 pss
    echo "**************************"
    echo "Test RSA Encrypt & Decrypt"
    rsa_encrypt_and_decrypt_test 3072 none
    rsa_encrypt_and_decrypt_test 2048 pkcs1
    rsa_encrypt_and_decrypt_test 2048 oaep

    if [ $DO_VALGRIND ]; then
        rm $TEMPFILE
    fi
}

main "$@"
