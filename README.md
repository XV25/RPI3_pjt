# RPI3_pjt

## Description du package

Le package est constitué de 3 dossiers (4 sur la branche devel) : 

### Test_C/v4l2grab-master

Contient toute la partie serveur, associée à la prise d'image sur la raspberry et à la communication de celles-ci aux clients connectés. 

Le programme principal s'appelle v4l2grab.c. Dedans se trouve la gestion de la caméra et des ordres des clients. 
Le programme gère uniquement la prise d'image, la connexion et la déconnexion des clients.
La méthode pour compiler, obtenir l'exécutable associé à ce programme (v4l2grab) et l'utiliser est présenté après.

Elle communique via deux LEDs : 

- Une LED sur le pin GPIO17, lié à la communication de l'état du serveur (allumé quand activé, éteint sinon).

- Une LED sur le pin GPIO18, lié à la communication de la prise de photo (allumé au début de la procédure, éteint à la fin).

Elle utilise deux librairies : 

- yuc.c, correspondant à la gestion hardware de la caméra.

- lblink.c, correspondant à une API pour la gestion des pins correspondant aux LED.

Elle est aussi linkée à une autre librairie (comm.c, correspondant à une API pour gérer les communications entre serveur et client). Mais par manque de temps, celle-ci n'est pas utilisée actuellement.

### client

Contient toute la partie client, associée aux ordres de prise d'image. Le client peut uniquement envoyer des ordres pour la prise d'image, ou demander à se déconnecter. 

Elle est sensé être linkée à une autre librairie (comm.c, correspondant à une API pour gérer les communications entre serveur et client). Mais par manque de temps, celle-ci n'est pas utilisée actuellement.

### Executables

Contient des exécutables déjà compilés.

client : correspond à l'exécutable associé au client. Utilisable sur Ubuntu 18.04.

client_rpi3 :  correspond à l'exécutable associé au client. Utilisable sur la raspberry 3.

v4l2grab : correspond à l'exécutable associé au serveur et à la caméra. Utilisable sur la raspberry 3.

### Test_RPI (branche devel)

Tentatives pour lancer un programme automatiquement au lancement de la raspberry. Inutilisable en l'état (manquement d'un package sur la raspberry). 

## Utilisation du package

### Lancement du conteneur pour la cross-compilation

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

### Récupération des fichiers nécessaires

On utilise find -name pour retrouver les fichiers souhaités: 

	find -name sdcard.img 

	--> /root/buildroot-precompiled-2017.08/output/images/sdcard.img

	find -name start_x.elf 

	--> /root/buildroot-precompiled-2017.08/output/build/rpi-firmware-685b3ceb0a6d6d6da7b028ee409850e83fb7ede7/boot/start_x.elf

	find -name fixup_x.dat 

	--> /root/buildroot-precompiled-2017.08/output/build/rpi-firmware-685b3ceb0a6d6d6da7b028ee409850e83fb7ede7/boot/fixup_x.dat

Une fois les fichiers trouvés, effectuer la commande de copie suivante pour chacun des fichiers :  

	docker cp a333cea88350:/root/buildroot-precompiled-2017.08/output/images/sdcard.img .


### Flashage de la carte SD 

Mettre la sd dans l'ordinateur

Trouver l'identifiant de la carte sd via la commande lsblk -fs --> ici, mmcblk0

Flasher la carte sd (ATTENTION : OPERATION TRÈS DELICATE : ÊTRE ABSOLUMENT SÛR QUE L'IDENTIFIANT DE LA CARTE SD SOIT CORRECT!)

	sudo dd if=sdcard.img of=/dev/mmcblk0

Copier start_x.elf et fixup_x.dat sur la 1e partition de la carte sd.

Modifier config.txt :

	nano config.txt

Ajouter : 

	start_x=1
	gpu_mem=128

Démonter les deux partitions de la carte sd.

Débrancher la carte sd.

NB : à cette étape, il est possible que la carte sd présente des problèmes (2e partition illisible). Si c'est le cas, utiliser la commande fsck /dev/<identifiant_carte_sd>.
Ici, cela correspond à /dev/mmcblk0.

### Lancement de la raspberry pi 

Comme vu dans les derniers TP : 

- brancher caméra à raspberry

- brancher raspberry à ethernet

- sur une breadboard, effectuer les branchements pour les deux LEDs utilisées sur les pins GPIO17 et GPIO18 

(le branchement doit être le suivant pour chaque LED : 

https://raspberry-pi.fr/wp-content/uploads/2019/05/raspberry-pi-led-939x528.png 

Avec le câble rouge correspondant au pin GPIO, et le câble noir au pin Ground)

- brancher les câbles dupont de l'adaptateur USB-TTL à la raspberry éteinte (attention à l'inversion des câbles, entre les pins RX-TX de la raspberry et de l'adaptateur)

- brancher le port usb de l'adaptateur à l'ordinateur

- trouver l'identifiant de l'adaptateur (commande dmseg). Ici, ttyUSB0

- connexion via minicom  : 

		sudo minicom -b 115200 -D /dev/ttyUSB0 

- désactiver le contrôle de l'hardware machine dans la configuration minicom : pour cela, Ctrl+A --> O, puis F pour désactiver le contrôle

- brancher la raspberry à l'alimentation.

Deux logins sont alors possibles : 

Utilisateur classique :

	Identifiant : user
	Mdp : user1*

Super-utilisateur : 

	Identifiant : root
	Mdp : root1*

Le changement de l'un à l'autre des logins se fait en ligne de commande, via la commande su.

- Prendre l'ip de la raspberry : 

	ipconfig

Ici, cela correspond à est 172.20.10.199

### Compilation

Pour utiliser le client sur un ordinateur : 

Vérifier l'option GCC : si celle-ci contient gcc, le makefile est utilisable. Sinon, remplacer l'actuelle option GCC par la ligne suivante : 

		GCC=gcc

Utiliser make : make
	       
On obtient l'exécutable client

### Cross-compilation

-Transférer les fichiers devant être exécutés sur la raspberry (et donc, compilés sur le docker)

		sudo docker v4l2grab-master/ a333cea88350:/home/

		sudo docker client/ a333cea88350:/home/

- Modifier la variable path, pour donner accès aux fichiers .sh dans ce path (dont ceux nécessaires pour la compilation)

		export PATH=$PATH:/root/buildroot-precompiled-2017.08/output/host/bin/

-Compiler les programmes

Pour v4l2grab-master : 

			* Utiliser autogen.sh : ./autogen.sh
	
		       * Utiliser configure avec la variable host correspondante à l'image flashée sur la raspberry (vu via buildroot) : ./configure --host=arm-linux

		       * Utiliser make : make

		       * On obtient l'exécutable v4l2grab

Pour client (si on souhaite l'exécuter sur la raspberry) : 
	      
	      * Vérifier l'option GCC : si celle-ci contient /root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc, le makefile est utilisable. Sinon, remplacer l'actuelle option GCC par la ligne suivante : 

GCC=/root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc

	      * Utiliser make : make
	       
	      * On obtient l'exécutable client

- Transférer sur l'ordinateur, dans un dossier différent du premier.

		sudo docker cp a333cea88350:/home/v4l2grab-master .

		sudo docker cp a333cea88350:/home/client .

- Transférer sur la raspberry 

		scp home/ehnla/Documents/UV_sysemb/rpi3_pjt/v4l2_doc/v4l2grab-master/v4l2grab user@172.20.10.199:/home/user/

		scp home/ehnla/Documents/UV_sysemb/rpi3_pjt/v4l2_doc/client/client user@172.20.10.199:/home/user/

### Lancement sur raspberry 

Utilisable uniquement en super-utilisateur (su si login user)

- Lancer le module kernel (comme dans le sujet)

		modprobe bcm2835-v4l2

- Vérifier la présence de device : 

		ls /dev/video0

- Vérifier le bon fonctionnement du serveur : 

		./v4l2grab -h

Normalement, l'ensemble des options du serveur doivent apparaître.

### Exécution des programmes

- Pour la partie serveur : l'ensemble des options sont fixées, mais celles-ci peuvent être modifiées au démarrage par l'utilisateur. Pour afficher l'ensemble des options modifiables, utiliser ./v4l2grab -h.

A titre d'exemple, une exécution classique est de la forme suivante : 

		./v4l2grab -d /dev/video0 -o try1.jpg -P 2400

Avec : 

-d pour l'accès à la caméra

-o pour l'enregistrement d'images du côté serveur (en format .jpg). L'image sera remplacée à chaque nouvel ordre de prise d'image de l'utilisateur. Utile pour le débuggage. 

-P pour le choix du port pour la communication serveur / client.

Le programme se stoppe via ctrl+C.

- Pour la partie client : 

Celui-ci se lance de la façon suivante : 

		./client <adresse_ip> <numéro_port>

L'ensemble des options s'affichent avec la commande h. 

Le programme se stoppe via la commade "bye" dans le terminal.
