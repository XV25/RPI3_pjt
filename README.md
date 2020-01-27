# RPI3_pjt

# Lancement du conteneur souhaité

1e lancement : 

docker pull pblottiere/embsys-rpi3-buildroot-video

docker run -it pblottiere/embsys-rpi3-buildroot-video /bin/bash

docker# cd /root

docker# ls

buildroot-precompiled-2017.08.tar.gz

docker# tar zxvf buildroot-precompiled-2017.08.tar.gz

Relancer : 

docker ps -a --> récupérer id container souhaité; ici, a333cea88350 

docker container start a333cea88350

docker exec -it a333cea88350 /bin/bash

# Récupération des fichiers nécessaires

On utilise find -name pour retrouver les fichiers souhaités: 
find -name sdcard.img --> ./buildroot-precompiled-2017.08/output/images/sdcard.img
find -name start_x.elf -->
./buildroot-precompiled-2017.08/output/build/rpi-firmware-685b3ceb0a6d6d6da7b028ee409850e83fb7ede7/boot/start_x.elf
find -name fixup_x.dat -->
./buildroot-precompiled-2017.08/output/build/rpi-firmware-685b3ceb0a6d6d6da7b028ee409850e83fb7ede7/boot/fixup_x.dat

Une fois les fichiers trouver, commande copie suivante : 

docker cp a333cea88350:./buildroot-precompiled-2017.08/output/images/sdcard.img .

Pour chaque fichier

# Flashage de la carte SD 

Mettre sd dans ordinateur

lsblk -fs pour trouver identifiant de carte sd --> ici, mmcblk0

sudo dd if=sdcard.img of=/dev/mmcblk0

Ensuite, étape très importante pour garder la sd intacte !! 

cp start_x.elf <path vers la 1e partition de la sd>
cp fixup_x.dat <path vers la 1e partition de la sd>

Modifier config.txt :

nano config.txt

Ajouter : 

start_x=1
gpu_mem=128

# Lancement de la raspberry pi 

Comme vu dans les derniers TP : 

- brancher caméra à raspberry
- brancher raspberry à ethernet
- brancher les câbles dupont de l'adaptateur USB-TTL à la raspberry éteinte (attention au changement entre les pins RX-TX)
- brancher le port usb de adapt à l'ordinateur
- trouver l'identifiant de adapt (commande dmseg). Ici, ttyUSB0
- connexion via minicom  : sudo minicom -b 115200 -D /dev/ttyUSB0 
- désactiver hardware machine dans configuration minicom
- brancher raspberry à alimentation

Identifiant : user
Mdp : user1*

Si besoin de passer root : 

su
Identifiant : root
Mdp : root1*

- Prendre ip de raspberry : 

ipconfig

Ici, est 172.20.10.199



# Cross-compilation

-Transférer fichiers concernés sur docker : 
sudo docker v4l2grab-master/ a333cea88350:/home/

- Modifier variable path, pour donner accès aux fichiers .sh dans ce path (dont ceux nécessaires pour la compilation)

export PATH=$PATH:/root/buildroot-precompiled-2017.08/output/host/bin/

-Utiliser autogen.sh

./autogen.sh

-Utiliser configure avec la variable host correspondante à l'image flashée sur la raspberry (vu via buildroot)

./configure --host=arm-linux

- Utiliser make

make

- Transférer sur ordinateur 

sudo docker cp a333cea88350:/home/v4l2grab-master .

- Transférer sur raspberry 

scp home/ehnla/Documents/UV_sysemb/rpi3_pjt/v4l2_doc/v4l2grab-master user@172.20.10.199:/home/user/

# Lancement sur raspberry 

- Lancement module kernel (ds sjt)

modprobe bcm2835-v4l2

- Vérifier présence de device : 

ls /dev/video0

- Vérifier bon fonctionnement de code test : 

./v4l2grab -d /dev/video0 -o image.jpg

Pour le code : rajouter l'option -P pour ajouter le numéro de port : 

./v4l2grab -d /dev/video0 -o try1.jpg -P 2400

En ssh : ./client fonctionne juste si exécuté en root (autrement, librairies pas bien linkées).

(Ou nécessaire de faire manip suivante : 

LD_LIBRARY_PATH=/usr/lib
export LD_LIBRARY_PATH
su 

Et vérifier que exécutable est bien linké avec librairies : ldd ./client ici )


Actuellement, du côté client, à chaque prise d'image, crée une image appelée client[nombre_image].jpg, supprime l'ancien client[nombre_image_precedente].jpg. La dernière image peut être récupéré au niveau de la raspberry. 

Bug actuel : la prise d'image a beaucoup de lag : lié au nombre de fps sur v4l2grab.c?
