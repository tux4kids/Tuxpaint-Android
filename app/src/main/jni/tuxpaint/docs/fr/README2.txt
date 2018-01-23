   brosses, tampons...

  Comment créer des brosses, des tampons, des polices et des images "starter"?

   Si vous voulez ajouter ou changer des choses telles que les brosses et les
   tampons utilisés par Tux Paint,  vous pouvez le faire simplement en
   ajoutant ou en enlevant des fichiers sur votre disque dur.

   NB : vous devrez redémarrer Tux Paint pour que les changements prennent
   effet.
    1. Les répertoires où Tux Paint range les différents éléments
    2. Comment créer des brosses?
    3. Comment créer des tampons?
    4. Comment créer des Images "starter"?
    5. Comment ajouter des polices?
    6. Importer des images pour les ouvrir dans Tux Paint.

1. Les répertoire où Tux Paint range les différents éléments.

  Les fichiers standards

   Tux Paint regarde dans ses répertoires de données pour trouver ses
   fichiers de configuration.

    Linux et Unix

   Où ces répertoires sont installés dépend de la valeur définie pour
   "DATA_PREFIX" quand Tux Paint est construite. Pour plus de détail voir
   INSTALL.txt.

   Par défaut le répertoire est :
    /usr/local/share/tuxpaint/

   Si vous l'avez installé à partir d'un package il est plus sûrement : 
    /usr/share/tuxpaint/

    Mac OS X

   Tux Paint range ces fichiers dans le répertoire :
    /Users/Joe/Library/Application Support/tuxpaint/ et non pas dans
    /Users/Joe/Library/preferences/ comme indiqué dans le texte en anglais.
   Attention aux fichiers cachés (par exemple  /Users/Joe/Library/Application
   Support/tuxpaint/saved/.thumbnail/ )

     Windows

   Tux Paint regarde dans un répertoire nommé 'data' situé dans le même
   répertoire que le programme exécutable. Ces le répertoire qui est créé
   lors de l'installation : 
    C:\Program Files\TuxPaint\data

  Fichiers personnels

   Vous pouvez aussi créer des brosses, des tampons, des polices et des
   images 'starter' dans votre propre répertoire où Tux Paint les trouvera.

    Linux et Unix

   Votre répertoire Tux Paint personnel est  "~/.tuxpaint/".

   C'est à dire que si votre répertoire home est "/home/karl", alors votre
   répertoire Tux Paint est "/home/karl/.tuxpaint/".

   Ne pas oublier le point (".") avant 'tuxpaint'!

    Mac OS X

   Dans la version anglaise rien est dit concernant Mac OS X. J'ai d'abord
   cru qu'il fallait faire comme pour linux, après tout OS X est un système
   UNIX; mais ce n'est pas le cas. En fait on peut créer les dossiers
   brushes, stamps, fonts et starters dans le dossier
    /Users/Joe/Library/Application Support/tuxpaint/ et cela fonctionne.

    Windows

   Votre répertoire Tux Paint personnel se nomme "userdata" et il est dans le
   même répertoire que l'exécutable :
    C:\Program Files\TuxPaint\userdata

2. Comment créer des brosses?

   Pour créer des brosses : il faut d'abord créer un dossier brushes, s'il
   n'existe pas, dans votre répertoire personnel de Tux Paint.
   Les brosses utilisées pour l'outil dessin et l'outil ligne dans Tux Paint
   sont de simple images PNG en niveau de gris.
   La couche alpha (transparence) de l'image PNG est utilisée pour déterminer
   la forme de la brosse, ce qui signifie que la forme peut-être anti-aliasée
   et même partiellement transparente. (L'anti-aliasing est une technique qui
   rend les bord d'une figure légèrement floue pour qu'on ait pas
   l'impression de voir une forme pixellisée).
   Les images de brosses ne doivent pas être plus grande que 40 pixel par 40.

   Une fois l'image PNG de la brosse crée il n'y a plus qu'à la sauvegarder
   dans le dossier brushes.

   NB : Si votre nouvelle brosse apparaît comme un rectangle (ou un carré)
   plein, c'est parce que vous avez oublié d'utiliser la transparence! Voir
   la documentation Qu'est qu'un PNG? Et comment en créer un? pour plus
   d'informations et de conseils.
    

3. Comment créer des tampons?

   Ils se rangent dans le répertoire stamps, s'il n'existe pas, dans votre
   répertoire personnel de Tux Paint.
   On peut créer des sous-dossiers dans son dossier stamps (par exemple
   /stamps/vacances/ et /stamps/animaux/ - ceux qui utilisent l'OS du coté
   obscur remplacent les / par des \.-).

   Un tampon, c'est une image au format PNG qui doit considérer les pixels
   blancs comme transparents (en fait c'est l'alpha qui détermine la
   transparence, c'est à dire que chaque pixel de l'image est plus ou moins
   transparent en fonction de la valeur alpha qui lui est allouée. Chaque
   point est plus ou moins transparent et laisse donc plus ou moins voir
   l'arrière plan.)

   tete_chien
   Pour des raisons démonstratives, le blanc apparaît en jaune dans le dessin
   ci-dessus.
   exemple 1 : seuls les contours de la tête sont marqué dans le dessin et on
   peut colorier autour et dedans
   exemple 2 : toute la tête est marquée, mais le tour du chien c'est
   transparent.
   exemple 3 : la transparence du dessin n'a pas été conservée le tampon est
   rectangulaire avec une tête de chien au milieu.
   Comment fait-on une image au format PNG? Personnellement j'utilise un
   logiciel open source de dessin qui s'appelle le GIMP (voir Qu'est qu'un
   PNG? Et comment en créer un?) ou photoshop element. D'autres logiciels
   sont capables de créer des images png. Le format se choisit au moment de
   l'enregistrement.
   La taille de l'image ne doit pas dépasser 100 pixels sur 100 (déjà une
   grosse image pour Tux Paint : mais attention çà veut dire que les détails
   du dessin peuvent ne pas passer donc prendre un dessin de base assez
   simple)
   Attention de bien enregistrer l'alpha en transparent. et attention dans le
   choix du nom : pas de caractères spéciaux ni accentués (Ils sont souvent
   responsables de problèmes.)

   Considérons maintenant que l'image tetechien.png. a été créée et qu'elle a
   été placée dans /stamps/animaux/
   On peut faire un texte d'explication qui apparaîtra dans le bas de la
   fenêtre de Tux Paint :
   ouvrir un éditeur de texte (par ex Text Edit sur Mac OS X, Kedit sur
   Linux, word pad sur Windows)
   première ligne description en anglais :"en .utf8= head of dog"
   deuxième ligne description en français "fr .utf8= tête de chien"
   (Si on veut mettre une description en espagnol 3° ligne :" es .utf8=
   cabeza de perro")
   On sauvegarde au format UTF8 (Paramétrez Text Edit pour qu'il créer de
   nouveaux documents au format simple text et choisir l'encodage UTF8 lors
   de l'enregistrement, sous Windows choisissez Plain text (ou simple texte))
   avec l'extension .txt (tetechien.txt) dans le dossier /stamps/animaux/

   On peut peux aussi associer un son à son image.
   On créer un son au format .WAV (AIFF sur Mac OS X dont on modifie
   l'extension .aif ou .aiff en .wav) nommé tetechien.wav dans le dossier 
   /stamps/animaux/. Si ce son est un mot, on peut créer toute une suite de
   traduction :
   par exemple
     * dog.wav, "son=dog";
     * dog_fr.wav, "son=chien";
     * dog_es.wav, "son=perro".
   On peut donner des instructions au logiciel pour qu'il gère d'une certaine
   manière le tampon. Pour cela il faut ouvrir un éditeur de texte et taper
   les instructions suivantes :
   colorable = si on écrit cette instruction le logiciel permettra à
   l'utilisateur de choisir la couleur au moment de l'utilisation (comme pour
   les pinceaux)
   tintable = si on écrit cette instruction l'image d'origine sera teintée
   par la couleur choisie par l'utilisateur; Seules les zones à plus de 25 %
   de saturations seront teintées.
   On peut si on veut rendre les gris non "teintables" en tapant notintgray.
   noflip = empêche la possibilité de retourner le tampon.
   nomirror = empêche la possibilité de mettre l'image du tampon en miroir.
   On sauvegardes en UTF8 mais avec l'extension .dat (tetechien.dat) dans le
   dossier /stamps/animaux/
   Un exemple de texte de paramétrage pour ma tête de chien :
       colorable
       noflip

   Enfin on peut créer une image miroir pré-enregistrée : par exemple si on a
   un camion de pompiers avec écrit service incendie, si on le laisse se
   mettre en miroir dans le logiciel normalement, on va avoir les mots écrit
   en miroir; on peut alors créer l'image miroir avec les mots bien écrits
   que tu nomme image_mirror.png dans le même dossier que image.png.

4. Comment créer des images "starter".

   Il faut créer un répertoire /starters/, s'il n'existe pas, dans votre
   répertoire personnel de Tux Paint.
   Les images de départ ('starter') apparaissent dans le dialogue d'ouverture
   de document, à coté des images que vous avez créés. Elles ont des boutons
   verts au lieu de bleu derrière.

   Contrairement à vos images sauvegardées, quand vous sélectionner et ouvrez
   un 'starter', en réalité vous créez une nouvelle image. Au lieu d'être
   blanche, cependant, la nouvelle image contient le contenu du 'starter'. De
   plus quand vous éditez votre nouvelle image, le contenu du 'starter'
   original l'affecte.

  Style livre de coloriage

   Le mode de 'starter' le plus basique ressemble à une image d'un livre à
   colorier. C'est une forme délimitée par des lignes à laquelle on peut
   ajouter des détails et des couleurs. Dans Tux Paint, quand vous dessinez,
   tapez du texte, utilisez les tampons, les lignes du dessins restent
   au-dessus de ce que vous dessinez. Vous pouvez effacer ce que vous
   rajoutez mais pas les lignes du 'starter'.

   Pour créer une telle image, dessinez simplement une forme en ligne dans un
   programme de dessin, rendez le reste transparent (ce qui deviendra blanc
   dans Tux Paint), et sauvegardez au format PNG dans le dossier /starters/.

  Style scène

   A coté du style livre de coloriage, vous pouvez aussi procurer comme
   'starter', un avant plan et un arrière plan séparé de l'image. Le principe
   est le même : on ne peut pas l'effacer, lui appliquer les effet magiques.
   On ne peut pas dessiner sur l'avant plan.

   Quand la gomme est appliquée à ce type d'image, au lieu de révéler du
   blanc elle révèle l'image d'arrière plan.

   En créant à la fois un avant plan et un arrière plan, on peut créer un
   'starter' simulant un effet de perspective. Imaginez un arrière plan
   représentant l'océan et un avant plan qui représente un récif. On peut
   ensuite dessiner ou tamponner des poissons dans l'image : ils apparaîtront
   dans l'océan mais jamais en avant du récif.

   Pour créer ce genre de starter, il faut créer un avant plan (avec
   transparence alpha) comme décrit précédemment, et le sauvegarder au format
   PNG dans le dossier /starters/. Ensuite créez une autre image sans
   transparence et la sauvegarder avec le même nom mais avec le suffixe
   "-back" ( Par exemple le récif du premier plan s'appelle reef.png et
   l'océan de l'arrière plan reef-back.png.)

   Le 'starter' doit avoir la même taille de canevas que Tux Paint. Par
   défaut c'est le mode 640x480, c'est à dire 448x376 pixels. (Si vous
   utilisez le mode 800x600, cela doit être 608x496 pixels.)

   Les 'starter' apparaissent avec un  bouton vert au début de la liste dans
   le dialogue d'ouverture.

   NB : Les 'starter' ne peuvent pas être sauvés comme tels à partir de Tux
   Paint car charger un starter, c'est vraiment comme créer une nouvelle
   image. (Au lieu d'être blanche, elle a quelque chose à l'intérieur. La
   commande 'sauvegarde' ne fait que créer une nouvelle image, tout comme si
   la commande 'nouvelle' avait été utilisée.)

   NB : Les 'starter' sont "attachés" aux images sauvegardées, via un petit
   fichier texte qui a le même nom que le dessin sauvegardé, mais au format
   .dat. Cela permet au premier plan et à l'arrière plan, s'ils existent, de
   continuer d'affecter le dessin après que Tux Paint ait été quitté, ou
   qu'une autre image ait été chargée ou démarrée. (En d'autres mots, si vous
   construisez un dessin à partir d'un 'starter', il sera toujours affecté
   par celui-ci.)

5. Comment ajouter des polices?

   Il faut là encore créer un dossier fonts, s'il n'existe pas, dans votre
   répertoire personnel de Tux Paint.
   Mettre dans ce dossier des polices de format TrueType. (Voir avec un
   gestionnaire de polices pour voir quel type de police on utilise). La
   police sera alors prise en charge dans Tux Paint,  avec 4 tailles
   différente proposées.

6. Importer des images pour les ouvrir dans Tux Paint.

   Comme le dialogue d'ouverture de Tux Paint ne nous montre que les dessins
   créés par lui-même, comment faire si vous voulez charger une autre image
   ou photographie dans Tux Paint pour l'éditer?

   Pour faire cela, vous devez convertir l'image en PNG ( voir Qu'est qu'un
   PNG? Et comment en créer un? ), et la placer dans le répertoire saved de
   Tux Paint (~/.tuxpaint/saved/ sous linux et UNIX, userdata\saved\ sous
   windows ~/Library/Application Support/tuxpaint/saved/ sous Mac OS X -et
   pas dans preferences comme indiqué dans la version anglaise-) Il faut
   aussi prévoir une icône pour apparaître dans le menu ouverture qui sera
   dans le répertoire  ~/.tuxpaint/saved/.thumb sous linux et
   UNIX, ~/Library/Application Support/tuxpaint/saved/.thumb sous Mac OS X,
   et je ne sais pas pour windows peut-être userdata\saved\thumb tout
   simplement.

  Utiliser 'tuxpaint-import'

   Les utilisateurs de Linux et d'UNIX peuvent utiliser le 'tuxpaint-import',
   un script shell qui s'installe quand vous installez Tux Paint. Il utilise
   quelques outils NetPBM pour convertir l'image  ("anytopnm"),  pour la
   retailler afin qu'elle entre dans le canevas de Tux Paint  ("pnmscale"),
   et la convertie en PNG  ("pnmtopng"). Il crée en même temps une icône pour
   afficher dans le menu ouverture.

   Il utilise aussi la commande date pour renommer l'image avec les
   conventions de Tux Paint qui nomme ses fichiers images sauvegardés en
   fonction de la date, de l'heure... (Souvenez-vous que vous ne demandez
   jamais un nom de fichier pour ouvrir ou sauvegarder une image!)

   Pour utiliser 'tuxpaint-import', lancez la commande à partir d'un shell et
   donnez lui le nom du fichier que vous voulez convertir.

   Il sera alors convertit et placé dans votre répertoire saved. (NB : Si
   vous faîtes cela pour un utilisateur différent - par exemple votre enfant,
   il faut exécuter la commande dans sa session.)

   Exemple:
    $ tuxpaint-import grandma.jpg
    grandma.jpg -> /home/username/.tuxpaint/saved/20020921123456.png
    jpegtopnm: WRITING A PPM FILE

   La première ligne ("tuxpaint-import grandma.jpg") est la commande à
   lancer. Les deux lignes suivantes sont les sorties ('output') pendant que
   le script s'exécute.

   Après le redémarrage de Tux Paint, l'image est alors disponible dans le
   dialogue d'ouverture. Il ne reste plus qu'à cliquer dessus l'icône.

   Pour les utilisateurs de Mac OS X contrairement à ce qui est dit dans la
   version anglaise, on peut aussi utiliser un script shell. Peut-être le
   script Tuxpaint-import est adaptable à Mac OS X, mais personnellement j'en
   ai récris un autre.
   Pré requis : il faut installer les outils NetPBM (à l'aide de fink et
   finkcommander par exemple) et il faut créer un répertoire ~/.tmp

   Voici donc le script que j'ai écrit

   #!/bin/bash

   # creation d'une variable date
     DATE=`date '+%Y%m%d%H%M%S'`

   # creation d'une variable de travail
     FICHIER_IMAGE=$1

   #creation et déplacement dans un fichier de travail
     cp $FICHIER_IMAGE $HOME/.tmp/

   #creation d'une image pour Thumbnail
     cp $HOME/.tmp/$FICHIER_IMAGE $HOME/.tmp/$FICHIER_IMAGE-t

   #creation de l'image au format png qui sera chargeable dans tux paint
     anytopnm $HOME/.tmp/$FICHIER_IMAGE | pnmscale --xysize 448 376 |
   pnmtopng  > $HOME/.tmp/$FICHIER_IMAGE.png

   # renommer en utilisant la variable date l'image png car le fichier doit
   # avoir le format suivant yyyymmddhhmmss.png
     mv $HOME/.tmp/$FICHIER_IMAGE.png $HOME/.tmp/$DATE.png

   #creation de l'image du dialogue d'ouverture
     anytopnm $HOME/.tmp/$FICHIER_IMAGE-t | pnmscale --xysize 92 56 |
   pnmtopng > $HOME/.tmp/$FICHIER_IMAGE-t.png

   # renommer en utilisant la variable date l'image png car le fichier doit
   # avoir le format suivant yyyymmddhhmmss-t.png
     mv $HOME/.tmp/$FICHIER_IMAGE-t.png $HOME/.tmp/$DATE-t.png

   # faire le menage
     rm $HOME/.tmp/$FICHIER_IMAGE
    

     rm $HOME/.tmp/$FICHIER_IMAGE-t
    
     mv $HOME/.tmp/$DATE.png $HOME/Library/Application\
   support/TuxPaint/saved/
     mv $HOME/.tmp/$DATE-t.png $HOME/Library/Application\
   support/TuxPaint/saved/.thumb

   exit 0
   Ce script s'utilise comme 'tuxpaint -import'

  Le faire Manuellement

   Les utilisateurs de Windows et de BeOS doivent actuellement faire la
   conversion manuellement.

   Lancez un programme qui est capable d'ouvrir votre image et de la
   convertir au format PNG. (Voir Qu'est qu'un PNG? Et comment en créer un?
   Pour avoir quelques suggestions concernant les programmes capables de
   faire cela.)

   Ouvrez l'image et réduisez sa taille à une taille inférieure ou égale à
   448X376 pixels.

   Sauvegardez l'image au format PNG. Il est fortement recommandé de nommer
   le fichier en utilisant la date et l'heure courante, puisque par
   convention Tux Paint utilise :
   AAAAMMJJhhmmss.png
       •     AAAA = Année
       •      MM = Mois (01-12)
       •      JJ = Jour (01-31)
       •      HH = Heure, au format 24h (00-23)
       •      mm = Minute (00-59)
       •      ss = Second (00-59)

    i.e. :
   20020921130500 - pour le 21 Septembre 2002 13h05m00

   Sauvegardez le PNG dans le dossier 'saved' de Tux Paint. (Voir plus haut)
