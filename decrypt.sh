#!/bin/bash

# On verifie que tout les parametres sont bien indiques par le user
if [ $2 ]
then
    # On verifie que le fichier contenant les morceaux existe
    if [ ! -e $1 ]
    then
        echo "Splits file container does not exist"
    else
        # On reconstitue le fichier chiffre et encode avec les morceaux
        # cat $1/x* > reconstitute64.ssl

        # Traitement du fichier recu == suppression du padding
        #sed -i 's/0000//g' $1

        # Decodage base64
        openssl base64 -in $1 -out "reconstitute.ssl" -d

        # Dechiffrement AES-256 via openssl
        openssl aes-256-ecb -out "archive.tar.gz" -in "reconstitute.ssl" -d -pass pass:$2
        # Decompression de l'archive obtenue
        tar xzvf archive.tar.gz

        # On finit par supprimer tout les fichiers temporaires utilises par le prog
        rm -f archive.tar.gz
        rm -f reconstitute.ssl
        #rm -f $1/x*
        #rm -f reconstitute64.ssl

        echo "Success files restored"
    fi
else
    echo "Usage : $0 <splits file container> <pass to decrypt>"
fi