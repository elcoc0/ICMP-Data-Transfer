#!/bin/bash

# On verifie que tout les parametres sont bien indiques par le user
if [ $4 ]
then
    # On verifie que les data a envoyer existent bien
    if [ ! -e $1 ]
    then
        echo "File to send does not exist"
    else
        # Compression des datas a envoyer
        tar czvf compress.tar.gz $1
        # Chiffrement AES-256 via openssl
        openssl aes-256-ecb -in "compress.tar.gz" -out "encrypt.ssl" -pass pass:$2
        # Encodage base64 pour eviter les erreurs de transfert
        openssl base64 -in "encrypt.ssl" -out "encrypt64.ssl"

        # On divise le fichier chiffre en differents morceaux, la taille des morceaux est indiquee par le user
        split -b $3 encrypt64.ssl

        #for file in x*
        #do
          #sed -i '1s/^/!/' "$file"
          #sed -i '1i!' "$file"
        #done

        # Si le dossier de destination n'existe on le cree
        if [ ! -d $4 ]
        then
            mkdir $4
        fi
        # On envoie tout les morceaux dans le dossier de destination
        mv x* $4

        # On finit par supprimer tout les fichiers temporaires utilises par le prog
        # rm -rf $1
        rm -f compress.tar.gz
        rm -f encrypt.ssl
        rm -f encrypt64.ssl
    fi
else
    echo "Usage : $0 <file to send> <pass to encrypt> <splits length (bytes)> <splits file container>"
fi