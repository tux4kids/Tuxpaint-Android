   brosses, tampons...

  Comment creer des brosses, des tampons, des polices et des images "starter"?

   Si vous voulez ajouter ou changer des choses telles que les brosses et les
   tampons utilises par Tux Paint,  vous pouvez le faire simplement en
   ajoutant ou en enlevant des fichiers sur votre disque dur.

   NB : vous devrez redemarrer Tux Paint pour que les changements prennent
   effet.
    1. Les repertoires ou Tux Paint range les differents elements
    2. Comment creer des brosses?
    3. Comment creer des tampons?
    4. Comment creer des Images "starter"?
    5. Comment ajouter des polices?
    6. Importer des images pour les ouvrir dans Tux Paint.

1. Les repertoire ou Tux Paint range les differents elements.

  Les fichiers standards

   Tux Paint regarde dans ses repertoires de donnees pour trouver ses
   fichiers de configuration.

    Linux et Unix

   Ou ces repertoires sont installes depend de la valeur definie pour
   "DATA_PREFIX" quand Tux Paint est construite. Pour plus de detail voir
   INSTALL.txt.

   Par defaut le repertoire est :
    /usr/local/share/tuxpaint/

   Si vous l'avez installe `a partir d'un package il est plus surement : 
    /usr/share/tuxpaint/

    Mac OS X

   Tux Paint range ces fichiers dans le repertoire :
    /Users/Joe/Library/Application Support/tuxpaint/ et non pas dans
    /Users/Joe/Library/preferences/ comme indique dans le texte en anglais.
   Attention aux fichiers caches (par exemple  /Users/Joe/Library/Application
   Support/tuxpaint/saved/.thumbnail/ )

     Windows

   Tux Paint regarde dans un repertoire nomme 'data' situe dans le meme
   repertoire que le programme executable. Ces le repertoire qui est cree
   lors de l'installation : 
    C:\Program Files\TuxPaint\data

  Fichiers personnels

   Vous pouvez aussi creer des brosses, des tampons, des polices et des
   images 'starter' dans votre propre repertoire ou Tux Paint les trouvera.

    Linux et Unix

   Votre repertoire Tux Paint personnel est  "~/.tuxpaint/".

   C'est `a dire que si votre repertoire home est "/home/karl", alors votre
   repertoire Tux Paint est "/home/karl/.tuxpaint/".

   Ne pas oublier le point (".") avant 'tuxpaint'!

    Mac OS X

   Dans la version anglaise rien est dit concernant Mac OS X. J'ai d'abord
   cru qu'il fallait faire comme pour linux, apres tout OS X est un systeme
   UNIX; mais ce n'est pas le cas. En fait on peut creer les dossiers
   brushes, stamps, fonts et starters dans le dossier
    /Users/Joe/Library/Application Support/tuxpaint/ et cela fonctionne.

    Windows

   Votre repertoire Tux Paint personnel se nomme "userdata" et il est dans le
   meme repertoire que l'executable :
    C:\Program Files\TuxPaint\userdata

2. Comment creer des brosses?

   Pour creer des brosses : il faut d'abord creer un dossier brushes, s'il
   n'existe pas, dans votre repertoire personnel de Tux Paint.
   Les brosses utilisees pour l'outil dessin et l'outil ligne dans Tux Paint
   sont de simple images PNG en niveau de gris.
   La couche alpha (transparence) de l'image PNG est utilisee pour determiner
   la forme de la brosse, ce qui signifie que la forme peut-etre anti-aliasee
   et meme partiellement transparente. (L'anti-aliasing est une technique qui
   rend les bord d'une figure legerement floue pour qu'on ait pas
   l'impression de voir une forme pixellisee).
   Les images de brosses ne doivent pas etre plus grande que 40 pixel par 40.

   Une fois l'image PNG de la brosse cree il n'y a plus qu'`a la sauvegarder
   dans le dossier brushes.

   NB : Si votre nouvelle brosse apparait comme un rectangle (ou un carre)
   plein, c'est parce que vous avez oublie d'utiliser la transparence! Voir
   la documentation Qu'est qu'un PNG? Et comment en creer un? pour plus
   d'informations et de conseils.
    

3. Comment creer des tampons?

   Ils se rangent dans le repertoire stamps, s'il n'existe pas, dans votre
   repertoire personnel de Tux Paint.
   On peut creer des sous-dossiers dans son dossier stamps (par exemple
   /stamps/vacances/ et /stamps/animaux/ - ceux qui utilisent l'OS du cote
   obscur remplacent les / par des \.-).

   Un tampon, c'est une image au format PNG qui doit considerer les pixels
   blancs comme transparents (en fait c'est l'alpha qui determine la
   transparence, c'est `a dire que chaque pixel de l'image est plus ou moins
   transparent en fonction de la valeur alpha qui lui est allouee. Chaque
   point est plus ou moins transparent et laisse donc plus ou moins voir
   l'arriere plan.)

   tete_chien
   Pour des raisons demonstratives, le blanc apparait en jaune dans le dessin
   ci-dessus.
   exemple 1 : seuls les contours de la tete sont marque dans le dessin et on
   peut colorier autour et dedans
   exemple 2 : toute la tete est marquee, mais le tour du chien c'est
   transparent.
   exemple 3 : la transparence du dessin n'a pas ete conservee le tampon est
   rectangulaire avec une tete de chien au milieu.
   Comment fait-on une image au format PNG? Personnellement j'utilise un
   logiciel open source de dessin qui s'appelle le GIMP (voir Qu'est qu'un
   PNG? Et comment en creer un?) ou photoshop element. D'autres logiciels
   sont capables de creer des images png. Le format se choisit au moment de
   l'enregistrement.
   La taille de l'image ne doit pas depasser 100 pixels sur 100 (dej`a une
   grosse image pour Tux Paint : mais attention c,`a veut dire que les
   details du dessin peuvent ne pas passer donc prendre un dessin de base
   assez simple)
   Attention de bien enregistrer l'alpha en transparent. et attention dans le
   choix du nom : pas de caracteres speciaux ni accentues (Ils sont souvent
   responsables de problemes.)

   Considerons maintenant que l'image tetechien.png. a ete creee et qu'elle a
   ete placee dans /stamps/animaux/
   On peut faire un texte d'explication qui apparaitra dans le bas de la
   fenetre de Tux Paint :
   ouvrir un editeur de texte (par ex Text Edit sur Mac OS X, Kedit sur
   Linux, word pad sur Windows)
   premiere ligne description en anglais :"en .utf8= head of dog"
   deuxieme ligne description en franc,ais "fr .utf8= tete de chien"
   (Si on veut mettre une description en espagnol 3DEG ligne :" es .utf8=
   cabeza de perro")
   On sauvegarde au format UTF8 (Parametrez Text Edit pour qu'il creer de
   nouveaux documents au format simple text et choisir l'encodage UTF8 lors
   de l'enregistrement, sous Windows choisissez Plain text (ou simple texte))
   avec l'extension .txt (tetechien.txt) dans le dossier /stamps/animaux/

   On peut peux aussi associer un son `a son image.
   On creer un son au format .WAV (AIFF sur Mac OS X dont on modifie
   l'extension .aif ou .aiff en .wav) nomme tetechien.wav dans le dossier 
   /stamps/animaux/. Si ce son est un mot, on peut creer toute une suite de
   traduction :
   par exemple
     * dog.wav, "son=dog";
     * dog_fr.wav, "son=chien";
     * dog_es.wav, "son=perro".
   On peut donner des instructions au logiciel pour qu'il gere d'une certaine
   maniere le tampon. Pour cela il faut ouvrir un editeur de texte et taper
   les instructions suivantes :
   colorable = si on ecrit cette instruction le logiciel permettra `a
   l'utilisateur de choisir la couleur au moment de l'utilisation (comme pour
   les pinceaux)
   tintable = si on ecrit cette instruction l'image d'origine sera teintee
   par la couleur choisie par l'utilisateur; Seules les zones `a plus de 25 %
   de saturations seront teintees.
   On peut si on veut rendre les gris non "teintables" en tapant notintgray.
   noflip = empeche la possibilite de retourner le tampon.
   nomirror = empeche la possibilite de mettre l'image du tampon en miroir.
   On sauvegardes en UTF8 mais avec l'extension .dat (tetechien.dat) dans le
   dossier /stamps/animaux/
   Un exemple de texte de parametrage pour ma tete de chien :
       colorable
       noflip

   Enfin on peut creer une image miroir pre-enregistree : par exemple si on a
   un camion de pompiers avec ecrit service incendie, si on le laisse se
   mettre en miroir dans le logiciel normalement, on va avoir les mots ecrit
   en miroir; on peut alors creer l'image miroir avec les mots bien ecrits
   que tu nomme image_mirror.png dans le meme dossier que image.png.

4. Comment creer des images "starter".

   Il faut creer un repertoire /starters/, s'il n'existe pas, dans votre
   repertoire personnel de Tux Paint.
   Les images de depart ('starter') apparaissent dans le dialogue d'ouverture
   de document, `a cote des images que vous avez crees. Elles ont des boutons
   verts au lieu de bleu derriere.

   Contrairement `a vos images sauvegardees, quand vous selectionner et
   ouvrez un 'starter', en realite vous creez une nouvelle image. Au lieu
   d'etre blanche, cependant, la nouvelle image contient le contenu du
   'starter'. De plus quand vous editez votre nouvelle image, le contenu du
   'starter' original l'affecte.

  Style livre de coloriage

   Le mode de 'starter' le plus basique ressemble `a une image d'un livre `a
   colorier. C'est une forme delimitee par des lignes `a laquelle on peut
   ajouter des details et des couleurs. Dans Tux Paint, quand vous dessinez,
   tapez du texte, utilisez les tampons, les lignes du dessins restent
   au-dessus de ce que vous dessinez. Vous pouvez effacer ce que vous
   rajoutez mais pas les lignes du 'starter'.

   Pour creer une telle image, dessinez simplement une forme en ligne dans un
   programme de dessin, rendez le reste transparent (ce qui deviendra blanc
   dans Tux Paint), et sauvegardez au format PNG dans le dossier /starters/.

  Style scene

   A cote du style livre de coloriage, vous pouvez aussi procurer comme
   'starter', un avant plan et un arriere plan separe de l'image. Le principe
   est le meme : on ne peut pas l'effacer, lui appliquer les effet magiques.
   On ne peut pas dessiner sur l'avant plan.

   Quand la gomme est appliquee `a ce type d'image, au lieu de reveler du
   blanc elle revele l'image d'arriere plan.

   En creant `a la fois un avant plan et un arriere plan, on peut creer un
   'starter' simulant un effet de perspective. Imaginez un arriere plan
   representant l'ocean et un avant plan qui represente un recif. On peut
   ensuite dessiner ou tamponner des poissons dans l'image : ils apparaitront
   dans l'ocean mais jamais en avant du recif.

   Pour creer ce genre de starter, il faut creer un avant plan (avec
   transparence alpha) comme decrit precedemment, et le sauvegarder au format
   PNG dans le dossier /starters/. Ensuite creez une autre image sans
   transparence et la sauvegarder avec le meme nom mais avec le suffixe
   "-back" ( Par exemple le recif du premier plan s'appelle reef.png et
   l'ocean de l'arriere plan reef-back.png.)

   Le 'starter' doit avoir la meme taille de canevas que Tux Paint. Par
   defaut c'est le mode 640x480, c'est `a dire 448x376 pixels. (Si vous
   utilisez le mode 800x600, cela doit etre 608x496 pixels.)

   Les 'starter' apparaissent avec un  bouton vert au debut de la liste dans
   le dialogue d'ouverture.

   NB : Les 'starter' ne peuvent pas etre sauves comme tels `a partir de Tux
   Paint car charger un starter, c'est vraiment comme creer une nouvelle
   image. (Au lieu d'etre blanche, elle a quelque chose `a l'interieur. La
   commande 'sauvegarde' ne fait que creer une nouvelle image, tout comme si
   la commande 'nouvelle' avait ete utilisee.)

   NB : Les 'starter' sont "attaches" aux images sauvegardees, via un petit
   fichier texte qui a le meme nom que le dessin sauvegarde, mais au format
   .dat. Cela permet au premier plan et `a l'arriere plan, s'ils existent, de
   continuer d'affecter le dessin apres que Tux Paint ait ete quitte, ou
   qu'une autre image ait ete chargee ou demarree. (En d'autres mots, si vous
   construisez un dessin `a partir d'un 'starter', il sera toujours affecte
   par celui-ci.)

5. Comment ajouter des polices?

   Il faut l`a encore creer un dossier fonts, s'il n'existe pas, dans votre
   repertoire personnel de Tux Paint.
   Mettre dans ce dossier des polices de format TrueType. (Voir avec un
   gestionnaire de polices pour voir quel type de police on utilise). La
   police sera alors prise en charge dans Tux Paint,  avec 4 tailles
   differente proposees.

6. Importer des images pour les ouvrir dans Tux Paint.

   Comme le dialogue d'ouverture de Tux Paint ne nous montre que les dessins
   crees par lui-meme, comment faire si vous voulez charger une autre image
   ou photographie dans Tux Paint pour l'editer?

   Pour faire cela, vous devez convertir l'image en PNG ( voir Qu'est qu'un
   PNG? Et comment en creer un? ), et la placer dans le repertoire saved de
   Tux Paint (~/.tuxpaint/saved/ sous linux et UNIX, userdata\saved\ sous
   windows ~/Library/Application Support/tuxpaint/saved/ sous Mac OS X -et
   pas dans preferences comme indique dans la version anglaise-) Il faut
   aussi prevoir une icone pour apparaitre dans le menu ouverture qui sera
   dans le repertoire  ~/.tuxpaint/saved/.thumb sous linux et
   UNIX, ~/Library/Application Support/tuxpaint/saved/.thumb sous Mac OS X,
   et je ne sais pas pour windows peut-etre userdata\saved\thumb tout
   simplement.

  Utiliser 'tuxpaint-import'

   Les utilisateurs de Linux et d'UNIX peuvent utiliser le 'tuxpaint-import',
   un script shell qui s'installe quand vous installez Tux Paint. Il utilise
   quelques outils NetPBM pour convertir l'image  ("anytopnm"),  pour la
   retailler afin qu'elle entre dans le canevas de Tux Paint  ("pnmscale"),
   et la convertie en PNG  ("pnmtopng"). Il cree en meme temps une icone pour
   afficher dans le menu ouverture.

   Il utilise aussi la commande date pour renommer l'image avec les
   conventions de Tux Paint qui nomme ses fichiers images sauvegardes en
   fonction de la date, de l'heure... (Souvenez-vous que vous ne demandez
   jamais un nom de fichier pour ouvrir ou sauvegarder une image!)

   Pour utiliser 'tuxpaint-import', lancez la commande `a partir d'un shell
   et donnez lui le nom du fichier que vous voulez convertir.

   Il sera alors convertit et place dans votre repertoire saved. (NB : Si
   vous faites cela pour un utilisateur different - par exemple votre enfant,
   il faut executer la commande dans sa session.)

   Exemple:
    $ tuxpaint-import grandma.jpg
    grandma.jpg -> /home/username/.tuxpaint/saved/20020921123456.png
    jpegtopnm: WRITING A PPM FILE

   La premiere ligne ("tuxpaint-import grandma.jpg") est la commande `a
   lancer. Les deux lignes suivantes sont les sorties ('output') pendant que
   le script s'execute.

   Apres le redemarrage de Tux Paint, l'image est alors disponible dans le
   dialogue d'ouverture. Il ne reste plus qu'`a cliquer dessus l'icone.

   Pour les utilisateurs de Mac OS X contrairement `a ce qui est dit dans la
   version anglaise, on peut aussi utiliser un script shell. Peut-etre le
   script Tuxpaint-import est adaptable `a Mac OS X, mais personnellement
   j'en ai recris un autre.
   Pre requis : il faut installer les outils NetPBM (`a l'aide de fink et
   finkcommander par exemple) et il faut creer un repertoire ~/.tmp

   Voici donc le script que j'ai ecrit

   #!/bin/bash

   # creation d'une variable date
     DATE=`date '+%Y%m%d%H%M%S'`

   # creation d'une variable de travail
     FICHIER_IMAGE=$1

   #creation et deplacement dans un fichier de travail
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
   convertir au format PNG. (Voir Qu'est qu'un PNG? Et comment en creer un?
   Pour avoir quelques suggestions concernant les programmes capables de
   faire cela.)

   Ouvrez l'image et reduisez sa taille `a une taille inferieure ou egale `a
   448X376 pixels.

   Sauvegardez l'image au format PNG. Il est fortement recommande de nommer
   le fichier en utilisant la date et l'heure courante, puisque par
   convention Tux Paint utilise :
   AAAAMMJJhhmmss.png
        o      AAAA = Annee
        o       MM = Mois (01-12)
        o       JJ = Jour (01-31)
        o       HH = Heure, au format 24h (00-23)
        o       mm = Minute (00-59)
        o       ss = Second (00-59)

    i.e. :
   20020921130500 - pour le 21 Septembre 2002 13h05m00

   Sauvegardez le PNG dans le dossier 'saved' de Tux Paint. (Voir plus haut)
