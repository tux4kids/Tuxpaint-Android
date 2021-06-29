                                   Tux Paint
                                 version 0.9.26

  Un programme simple pour les enfants

      Copyright &copie; 2002-2021 par divers contributeurs; voir AUTHORS.
                            http://www.tuxpaint.org/

                                 juin 28, 2021

     ----------------------------------------------------------------------

   +-----------------------------------------------+
   |Table des matières                             |
   |-----------------------------------------------|
   |  * À propos de Tux Paint                      |
   |  * Utiliser Tux Paint                         |
   |       * Lancement de Tux Paint                |
   |       * Écran titre                           |
   |       * Écran principal                       |
   |       * Outils disponibles                    |
   |            * Outils de dessin                 |
   |            * Autres contrôles                 |
   |  * Chargement d'autres images dans Tux Paint  |
   |  * Lectures complémentaires                   |
   |  * Comment obtenir de l'aide                  |
   |  * Comment participer                         |
   +-----------------------------------------------+

     ----------------------------------------------------------------------

                             À propos de Tux Paint

Qu'est-ce que "Tux Paint" ?

       Tux Paint est un programme de dessin gratuit conçu pour les jeunes
       enfants (enfants de 3 ans et plus). Il possède une interface simple et
       facile à utiliser, des effets sonores amusants et une mascotte de
       dessin animé pour encourager et guider les enfants lorsqu'ils
       utilisent le programme. Sont fournis une toile vierge et un ensemble
       d'outils de dessin pour aider votre enfant à être créatif.

Licence :

       Tux Paint est un projet Open Source, un logiciel libre publié sous la
       licence publique générale GNU (GPL). Il est gratuit et le «code
       source» du programme est disponible. (Cela permet à d'autres d'ajouter
       des fonctionnalités, de corriger des bogues et d'utiliser des parties
       du programme dans leur propre logiciel sous GPL.)

       Voir COPYING.txt pour le texte complet de la licence GPL.

Objectifs :

       Facile et amusant
               Tux Paint se veut un simple programme de dessin pour les
               jeunes enfants. Il ne s'agit pas d'un programme de dessin à
               usage général. Il se veut amusant et facile à utiliser. Les
               effets sonores et un personnage de dessin animé permettent à
               l'utilisateur de savoir ce qui se passe tout en le
               divertissant. Il existe également des pointeurs de souris, de
               style dessin animé, extra-larges.

       Flexibilité
               Tux Paint est extensible. Les brosses et les formes de "tampon
               en caoutchouc" peuvent être déposées et retirées. Par exemple,
               un enseignant peut apporter une collection de formes animales
               et demander à ses élèves de dessiner un écosystème. Chaque
               forme peut avoir un son qui l'accompagne et un texte est
               affiché lorsque l'enfant sélectionne la forme.

       Portabilité
               Tux Paint est portable sur diverses plates-formes
               informatiques: Windows, Macintosh, Linux, etc. L'interface est
               la même sur toutes. Tux Paint fonctionne correctement sur les
               systèmes plus anciens (comme un Pentium 133), et peut être
               modifié pour mieux fonctionner sur des systèmes lents.

       Simplicité
               Il n'y a pas d'accès direct à la complexité sous-jacente de
               l'ordinateur. L'image en cours est conservée lorsque le
               programme se ferme et réapparaît au redémarrage.
               L'enregistrement d'images ne nécessite pas la création de noms
               de fichiers ou l'utilisation du clavier. L'ouverture d'une
               image se fait en la sélectionnant dans une collection de
               vignettes. L'accès à d'autres fichiers sur l'ordinateur est
               restreint.

     ----------------------------------------------------------------------

                               Utiliser Tux Paint

Lancement de Tux Paint

  Utilisateurs de Linux/Unix

         Tux Paint devrait avoir mis une icône de lanceur dans vos menus KDE
         et / ou GNOME, sous «Graphiques».

         Autrement, vous pouvez exécuter la commande suivante à une invite du
         shell (par exemple, "$") :

           $ tuxpaint

         Si des erreurs se produisent, elles seront affichées sur le terminal
         ("stderr").

     ----------------------------------------------------------------------

  Utilisateurs de Windows

                                                      [Icône pour Tux Paint]  
                                                            Tux Paint         

         Si vous avez installé Tux Paint sur votre ordinateur en utilisant le
         'Tux Paint Installer', il vous aura demandé si vous vouliez un
         raccourci de menu 'Démarrer' et / ou un raccourci sur le bureau. Si
         vous avez accepté, vous pouvez simplement exécuter Tux Paint à
         partir de la section 'Tux Paint' de votre menu 'Démarrer' (par
         exemple, sous «Tous les programmes» sous Windows XP), ou en
         double-cliquant sur l'icône «Tux Paint» sur votre bureau .

         Si vous avez installé Tux Paint en utilisant le téléchargement
         'ZIP-file', ou si vous avez utilisé 'Tux Paint Installer', mais que
         vous avez choisi de ne pas installer de raccourcis, vous devrez
         double-cliquer sur l'icône "tuxpaint.exe" dans le dossier "Tux
         Paint" de votre ordinateur.

         Par défaut, le 'Tux Paint Installer' mettra le dossier de Tux Paint
         dans "C:\Program Files\",ou bien vous avez pu changer cela lors de
         l'exécution du programme d'installation.

         Si vous avez utilisé le téléchargement de 'ZIP-file', le dossier de
         Tux Paint sera là où vous l'avez placé lorsque vous avez décompressé
         le fichier ZIP.

     ----------------------------------------------------------------------

  Utilisateurs de macOS

         Double-cliquez simplement sur l'icône "Tux Paint".

     ----------------------------------------------------------------------

Écran titre

       Lors du premier chargement de Tux Paint, un écran avec titre et
       crédits apparaîtra.

                                 [Écran-titre]

       Une fois le chargement terminé, appuyez sur une touche ou cliquez sur
       la souris pour continuer. (Ou, après environ 30 secondes,
       l'écran-titre disparaîtra automatiquement.)

     ----------------------------------------------------------------------

Écran principal

       L'écran principal est divisé en plusieurs sections :

       Côté gauche : la barre d'outils

               La barre d'outils contient les commandes pour dessiner et
               éditer.

  [Outils : Peindre, Tampon, Lignes, Formes, Texte, Magie, Étiquette, Défaire,
        Refaire, Gomme, Nouveau, Ouvrir, Sauvegarder, Imprimer, Quitter]

       Milieu : Toile pour Dessiner

               La plus grande partie de l'écran, au centre, est la toile de
               dessin. C'est évidemment là que vous dessinerez !

                                    [Toile]

               Remarque: la taille de la toile de dessin dépend de la taille
               de Tux Paint. Vous pouvez modifier la taille de Tux Paint à
               l'aide de l'outil de configuration tuxpaint-config, ou par
               d’autres moyens. Consultez la documentation OPTIONS pour plus
               de détails.

       Côté droit : sélecteur

               En fonction de l'outil en cours d'utilisation, le sélecteur
               affiche différentes choses. Par exemple, lorsque l'outil
               Pinceau est sélectionné, il affiche les différents pinceaux
               disponibles. Lorsque l'outil Tampon en caoutchouc est
               sélectionné, il affiche les différentes formes que vous pouvez
               utiliser.

               [Sélecteurs - Pinceaux, Lettres, Formes, Tampons]

       En bas : couleurs

               Une palette de couleurs disponibles s'affiche en bas de
               l'écran.

[Couleurs - Noir, Blanc, Rouge, Rose, Orange, Jaune, Vert, Cyan, Bleu, Pourpre,
                                  Brun, Gris]

               À l'extrême droite se trouvent deux options de choix de
               couleur, le "sélecteur de couleurs", qui a le contour d'un
               compte-gouttes, et vous permet de choisir une couleur trouvée
               dans votre dessin, et la palette arc-en-ciel, qui vous permet
               de choisir une couleur dans une boîte contenant des milliers
               de couleurs.

               (REMARQUE: vous pouvez définir vos propres couleurs. Voir la
               documentation " Options ".)

       En bas : zone d'aide

               Tout en bas de l'écran, Tux, le pingouin Linux, fournit des
               conseils et d'autres informations pendant que vous dessinez.

(Par exemple : Choisis une forme. Clique pour démarrer le dessin , fais glisser
  et continue jusqu'à la taille désirée. Déplace-toi pour la faire tourner, et
                             clique pour dessiner.)

     ----------------------------------------------------------------------

Outils disponibles

  Outils de dessin

         Outil "Peinture" (pinceau)

                 L'outil Pinceau vous permet de dessiner à main levée, en
                 utilisant différents pinceaux (choisis dans le sélecteur à
                 droite) et couleurs (choisis dans la palette de couleurs qui
                 est en bas).

                 Si vous maintenez le bouton de la souris enfoncé et déplacez
                 la souris, elle dessine au fur et à mesure que vous vous
                 déplacez.

                 Pendant que vous dessinez, un son est joué. Plus la brosse
                 est grosse, plus le son est grave.

     ----------------------------------------------------------------------

         Outil "Tampon" (tampon de caoutchouc)

                 L'outil Tampon montre un ensemble de tampons en caoutchouc
                 ou d'autocollants. Il vous permet de coller des images
                 pré-dessinées ou photographiques (comme l'image d'un cheval,
                 d'un arbre ou de la lune) dans votre dessin.

                 Lorsque vous déplacez la souris sur le canevas, une forme
                 suit la souris, indiquant où le tampon sera placé, ainsi que
                 sa taille.

                 Il peut y avoir de nombreuses catégories de timbres (par ex.
                 animaux, plantes, espace extra-atmosphérique, véhicules,
                 personnes, etc.). Utilisez les flèches gauche et droite pour
                 parcourir les différentes collections.

                 Avant de `` tamponner '' une image sur votre dessin,
                 différents effets peuvent parfois être appliqués (en
                 fonction du tampon) :

                    * Certains tampons peuvent être colorés ou teintés. Si la
                      palette de couleurs sous le canevas est activée, vous
                      pouvez cliquer sur les couleurs pour changer la teinte
                      ou la couleur du tampon avant de le placer dans le
                      dessin.
                    * Les tampons peuvent être rétrécis et agrandis, en
                      cliquant dans l'ensemble de barres (de forme
                      triangulaire) en bas à droite; plus la barre est
                      grande, plus le tampon apparaîtra grand sur votre
                      dessin.
                    * De nombreux tampons peuvent être retournés
                      verticalement ou affichés sous forme d'image miroir à
                      l'aide des boutons de commande en bas à droite.

                 Les tampons peuvent avoir un effet sonore et / ou une
                 description orale (parlés). Les boutons en bas à gauche
                 (près de Tux, le pingouin Linux) vous permettent de rejouer
                 les effets sonores et la description du tampon actuellement
                 sélectionné.

                 (REMARQUE: Si l'option "nostampcontrols" est définie, Tux
                 Paint n'affichera pas les commandes Miroir, Retourner,
                 Réduire et Agrandir pour les tampons. Voir la documentation
                 " Options".)

     ----------------------------------------------------------------------

         Outil "Lignes"

                 Cet outil vous permet de dessiner des lignes droites à
                 l'aide des différents pinceaux et couleurs que vous utilisez
                 habituellement.

                 Cliquez sur la souris et maintenez-la enfoncée pour choisir
                 le point de départ de la ligne. Au fur et à mesure que vous
                 déplacez la souris, une fine ligne «élastique» indiquera là
                 où la ligne sera dessinée.

                 Relâchez la souris pour terminer la ligne. On entend alors
                 le son "sproing !".

     ----------------------------------------------------------------------

         Outil "Formes"

                 Cet outil vous permet de dessiner des formes simples
                 remplies ou non remplies.

                 Choisissez une forme dans le sélecteur de droite (cercle,
                 carré,ovale, etc.).

                 Utilisez les options en bas à droite pour choisir le
                 comportement de l'outil :

                      Formes à partir du centre
                              La forme se développe à partir de l'endroit où
                              vous avez cliqué initialement et sera centrée
                              autour de cette position (C'était le seul
                              comportement de Tux Paint jusqu'à la version
                              0.9.24.)

                      Formes à partir d'un coin
                              La forme se développe à partir d'un coin depuis
                              l'endroit où vous avez cliqué initialement. Il
                              s'agit de la méthode par défaut de la plupart
                              des autres logiciels de dessin traditionnels.
                              (Cette option a été ajoutée à partir de la
                              version 0.9.25 de Tux Paint.)

                 Remarque : si les contrôles de forme sont désactivés (par
                 exemple, avec l'option "noshapecontrols"), il n'y aura pas
                 de contrôle et la méthode "formes à partir du centre" sera
                 utilisée.

                 Dans le dessin, cliquez sur la souris et maintenez-la pour
                 étirer la forme à partir de l'endroit où vous avez cliqué.
                 Certaines formes peuvent changer de proportion (par exemple,
                 le rectangle et l'ovale peuvent être plus larges que hauts
                 ou plus hauts que larges), d'autres pas (par exemple, carré
                 et cercle).

                 Relâchez la souris lorsque vous avez terminé l'étirement.

                      Mode normal

                              Vous pouvez maintenant déplacer la souris sur
                              le dessin pour faire pivoter la forme.

                              Cliquez à nouveau sur le bouton de la souris et
                              la forme sera dessinée avec la couleur en
                              cours.

                      Mode de Formes Simples
                              Si les formes simples sont activées (par ex.
                              avec l'option "simpleshapes"), la forme sera
                              dessinée sur lorsque vous relâcherez le bouton
                              de la souris. (Il n'y a pas de rotation.)

     ----------------------------------------------------------------------

         Outils "Texte" et "Étiquette"

                 Choisissez une police (parmi les «Lettres» disponibles sur
                 la droite) et une couleur (dans la palette de couleurs en
                 bas). Cliquez sur l'écran et un curseur apparaîtra. Tapez un
                 texte et il apparaîtra à l'écran.

                 Appuyez sur [Enter] ou [Return] et le texte sera inclus dans
                 l'image et le curseur se déplacera d'une ligne vers le bas.

                 Sinon, appuyez sur [Tab] et le texte sera inclus dans
                 l'image, mais le curseur se déplacera vers la droite du
                 texte, plutôt que vers le bas d'une ligne et vers la gauche.
                 (Cela peut être utile pour créer une ligne de texte avec des
                 couleurs, des polices, des styles et des tailles variés.)

                 Cliquer ailleurs dans l'image alors que l'entrée de texte
                 est toujours active entraîne le déplacement de la ligne de
                 texte actuelle vers cet emplacement (et vous pouvez
                 continuer à la modifier).

                      "Texte" par rapport à "Étiquette"

                              L' outil Texte est l'outil de saisie de texte
                              original de Tux Paint. Le texte saisi à l'aide
                              de cet outil ne peut pas être modifié ou
                              déplacé ultérieurement, car il fait partie du
                              dessin. Cependant, comme le texte fait partie
                              de l'image, il peut être dessiné ou modifié à
                              l'aide des effets de l'outil Magie (par
                              exemple, taché, teinté, gaufré, etc.)

                              Lors de l'utilisation de l' outil Étiquette
                              (qui a été ajouté à Tux Paint dans la version
                              0.9.22), le texte `` flotte '' sur l'image, et
                              les détails de l'étiquette (le texte, la
                              position de l'étiquette, le choix de la police
                              et la couleur ) sont stockés séparément. Cela
                              permet à l'étiquette d'être repositionnée ou
                              modifiée ultérieurement.

                              L' outil Étiquette peut être désactivé (par
                              exemple, en sélectionnant "Désactiver l'outil
                              'Label'" dans Tux Paint Config ou bien en
                              exécutant Tux Paint en ligne de commande avec
                              l'option "nolabel").

                      Saisie de caractères internationaux

                              Tux Paint permet de saisir des caractères dans
                              différentes langues. La plupart des caractères
                              latins ( A - Z , ñ , è , etc...) peuvent être
                              saisis directement. Certaines langues exigent
                              que Tux Paint soit commuté dans un mode
                              d'entrée alternatif avant la saisie, et
                              certains caractères doivent être composés en
                              utilisant plusieurs touches.

                              Lorsque les paramètres régionaux de Tux Paint
                              sont définis sur l'une des langues fournissant
                              des modes de saisie alternatifs, une touche est
                              utilisée pour parcourir le ou les modes soit
                              normaux (caractère latin) soit spécifiques aux
                              paramètres régionaux.

                              Les paramètres régionaux actuellement pris en
                              charge, les méthodes de saisie disponibles et
                              la touche pour basculer ou faire défiler les
                              modes sont répertoriés ci-dessous. Remarque :
                              de nombreuses polices n'incluent pas tous les
                              caractères pour toutes les langues, vous
                              devriez donc parfois changer de police pour
                              voir les caractères que vous essayez de saisir.

                                 * Japonais -- Hiragana romanisé et Katakana
                                   romanisé -- touche [Alt] droite
                                 * Korean — Hangul 2-Bul — touche [Alt]
                                   droite or touche [Alt] gauche
                                 * Chinois traditionnel — touche [Alt] droite
                                   or touche [Alt] gauche
                                 * Thai — touche [Alt] droite

                      Clavier virtuel sur écran

                              Un clavier virtuel sur écran (optionnel) est
                              disponible pour les outils "Texte" et
                              "Étiquette", qui peut présenter une palette de
                              dispositions et de création de caractères (par
                              ex "a" et "e" pour "æ"). Voir les documents
                              "Options" et "Extension de Tux Paint" pour plus
                              d'informations.

     ----------------------------------------------------------------------

         Outil "Remplir"

                 L'outil «Remplir» «remplit» une zone contiguë de votre
                 dessin avec une couleur unie de votre choix. Trois options
                 de remplissage sont offertes :
                    * Solide — cliquez une fois pour remplir une zone avec
                      une couleur unie.
                    * Linéaire—cliquez et faites glisser pour remplir une
                      zone avec une couleur qui s'atténue au fur et à mesure
                      dans la direction où vous déplacez la souris.
                    * Radial—cliquez une fois pour remplir une zone avec une
                      couleur qui s'atténue graduellement, à partir de
                      l'endroit où vous avez cliqué.

                 Remarque: avant Tux Paint 0.9.24, il s'agissait d'un outil
                 "magique" (voir ci-dessous). Remarque : avant Tux Paint
                 0.9.26, cet outil n'offrait que la méthode 'Solide' pour le
                 remplissage.

     ----------------------------------------------------------------------

         Outil "Magie" (Effets spéciaux)

                 L'outil «Magie» est en fait un ensemble d'outils spéciaux.
                 Sélectionnez l'un des effets «magiques» dans le sélecteur de
                 droite. Ensuite, selon l'outil, vous pouvez soit cliquer et
                 faire glisser dans l'image, et / ou simplement cliquer une
                 fois sur l'image pour appliquer l'effet.

                 Si l'outil peut être utilisé en cliquant et en faisant
                 glisser, un bouton «peinture» sera disponible sur la gauche,
                 sous la liste des outils «magiques» sur le côté droit de
                 l'écran. Si l'outil peut affecter toute l'image en entier,
                 un bouton «Image entière» sera disponible sur la droite.

                 Voir les instructions pour chaque outil 'Magie' (dans le
                 dossier 'magic-docs').

     ----------------------------------------------------------------------

         Outil "Gomme"

                 Cet outil est similaire au pinceau. Partout où vous cliquez
                 (ou cliquez et faites glisser), l'image sera effacée. (Cela
                 peut être du blanc, une autre couleur ou une image
                 d'arrière-plan, selon l'image.)

                 Un certain nombre de tailles de gommes sont disponibles,
                 soit rondes soit carrées.

                 Lorsque vous déplacez la souris, un contour carré suit le
                 pointeur, indiquant quelle partie de l'image sera effacée en
                 blanc.

                 Au fur et à mesure que vous effacez, un grincement est émis.

     ----------------------------------------------------------------------

  Autres contrôles

         Commande "Défaire"

                 En cliquant cet outil annulera la dernière action. Vous
                 pouvez même annuler plus d'une fois !

                 Remarque : vous pouvez également appuyer [Control] + [Z] sur
                 le clavier pour Défaire.

     ----------------------------------------------------------------------

         Commande "Refaire"

                 Cliquez sur cet outil pour refaire l'action de dessin que
                 vous venez de «annuler» avec le bouton «Défaire».

                 Tant que vous ne dessinez plus, vous pouvez refaire autant
                 de fois que vous avez défait !

                 Remarque : vous pouvez également appuyer [Control] + [R] sur
                 le clavier pour Refaire.

     ----------------------------------------------------------------------

         Commande "Nouveau"

                 Cliquez sur le bouton "Nouveau" pour démarrer un nouveau
                 dessin. Une boîte de dialogue apparaîtra, avec laquelle vous
                 pouvez choisir de commencer une nouvelle image en utilisant
                 une couleur d'arrière-plan unie, ou en utilisant une image
                 'Starter' ou 'Template' (voir ci-dessous). On vous demandera
                 d'abord si vous voulez vraiment faire cela.

                 Remarque : vous pouvez également appuyer [Control] + [N] sur
                 le clavier pour commencer un nouveau dessin.

                 Images de "Démarrage" et images "Modèle"

                   Les "Images de démarrage" se comportent comme une page
                   d'un livre de coloriage - un contour noir et blanc d'une
                   image, que vous pouvez ensuite colorier, et le contour
                   noir reste intact - ou comme une photographie 3D, où vous
                   dessinez entre une couche de premier plan et une
                   d'arrière-plan.

                   Les "Images modèle" sont semblables, mais fournissent
                   simplement un dessin d'arrière-plan sur lequel travailler.
                   Contrairement aux «Images de démarrage», rien de ce que
                   vous dessinerez ne restera au premier plan.

                   Lorsque vous utiliserez l'outil «Gomme», l'image d'origine
                   du «Démarrage» ou du «Modèle» réapparaîtra. Les outils
                   magiques "Retourner" et "Miroir" affecteront aussi bien
                   l'orientation de "l'image de démarrage" que celle de
                   "l'image Modèle".

                   Lorsque vous chargez une 'Image modèle' ou 'image modèle',
                   dessinez dessus, puis cliquez sur 'Sauvegarder', cela crée
                   un nouveau fichier image - il n'écrase pas l'original,
                   vous pouvez donc l'utiliser à nouveau plus tard (en y
                   accédant depuis la boîte de dialogue 'Nouveau').

     ----------------------------------------------------------------------

         Commande "Ouvrir"

                 Cela vous montre une liste de toutes les images que vous
                 avez enregistrées. S'il y en a plus que ce que peut contenir
                 l'écran, utilisez les flèches «Haut» et «Bas» en haut et en
                 bas de la liste pour faire défiler la liste des images.

                 Cliquez sur une image pour la sélectionner, puis ...

                      * Cliquez sur le bouton vert "Ouvrir" en bas à gauche
                        de la liste pour charger l'image sélectionnée.

                        (Vous pouvez également double-cliquer sur l'icône
                        d'une image pour la charger.)

                      * Cliquez sur le bouton marron "Effacer" (poubelle) en
                        bas à droite de la liste pour effacer l'image
                        sélectionnée. (Il vous sera demandé de confirmer.)

                        Remarque : à partir de la version 0.9.22, l'image
                        sera placée dans la corbeille de votre bureau,
                        uniquement sous Linux.

                      * Cliquez sur le bouton "Exporter" près du coin
                        inférieur droit pour exporter l'image vers votre
                        dossier d'exportation. (par exemple,
                        "~/Pictures/TuxPaint/")

                      * Cliquez sur le bouton bleu "Diapositives" (projecteur
                        de diapositives) en bas à gauche pour passer en mode
                        diaporama. Voir «Diapositives », ci-dessous, pour
                        plus de détails.

                      * Ou cliquez sur le bouton fléché rouge «Retour» en bas
                        à droite de la liste pour annuler et revenir à
                        l'image que vous étiez en train de dessiner.

                 Si vous choisissez d'ouvrir une image et que votre dessin
                 actuel n'a pas été enregistré, il vous sera demandé si vous
                 souhaitez l'enregistrer ou non. (Voir "Enregistrer,"
                 ci-dessous.)

                 Remarque : vous pouvez également appuyer [Control] + [O] sur
                 le clavier pour afficher la boîte de dialogue "Ouvrir".

     ----------------------------------------------------------------------

         Commande "Sauvegarder"

                 Pour sauvegarder votre image en cours.

                 Si vous ne l'avez pas enregistré auparavant, il créera une
                 nouvelle entrée dans la liste des images enregistrées.
                 (c'est-à-dire qu'il créera un nouveau fichier)

                 Remarque : il ne vous demandera rien (par exemple, un nom de
                 fichier). Il enregistrera simplement l'image et fera le
                 bruit d'un obturateur d'appareil photographique.

                 Si vous avez déjà enregistré l'image, ou s'il s'agit d'une
                 image que vous venez de charger à l'aide de la commande
                 "Ouvrir", il vous sera d'abord demandé si vous voulez
                 écraser l'ancienne version ou bien créer une nouvelle entrée
                 (un nouveau fichier).

                 Remarque : si les options " saveover" ou " saveovernew" sont
                 déjà définies, il ne sera rien demandé avant de sauvegarder.
                 Voir la documentation Options.

                 Remarque : vous pouvez également appuyer [Control] + [S] sur
                 le clavier pour sauvegarde.

     ----------------------------------------------------------------------

         Commande "Imprimer"

                 Cliquez sur ce bouton et votre image sera imprimée !

                 Sur la plupart des plates-formes, vous pouvez également
                 maintenir la touche [Alt] (appelée [Option] sur Mac) tout en
                 cliquant sur le bouton «Imprimer» pour obtenir une boîte de
                 dialogue d'impression. Notez que cela pourrait ne pas
                 fonctionner si vous exécutez Tux Paint en mode plein écran.
                 Voir ci-dessous.

                      Désactivation de l'impression

                              On peut définir une option "noprint", ce qui
                              entraînera la désactivation du bouton
                              "Imprimer".

                              Voir la documentation sur "Options".

                      Restreindre l'impression

                              Si l'option "printdelay" a été utilisée, vous
                              ne pouvez imprimer — qu'une fois toutes les x
                              secondes, tel que vous l'avez défini.

                              Par exemple, avec "printdelay=60" dans le
                              fichier de configuration de Tux Paint, vous ne
                              pouvez imprimer qu'une fois par minute.

                              Voir la documentation sur "Options".

                      Commandes d'impression

                              (Linux et Unix uniquement)

                              Tux Paint imprime en générant une
                              représentation PostScript du dessin et en
                              l'envoyant à un programme externe. Par défaut,
                              le programme est :

                                lpr

                              Cette commande peut être modifiée en
                              définissant la valeur "printcommand" dans le
                              fichier de configuration de Tux Paint.

                              Si la touche "[Alt]" du clavier est enfoncée
                              tout en cliquant sur le bouton «Imprimer», et
                              tant que vous n'êtes pas en mode plein écran,
                              un programme alternatif est exécuté. Par
                              défaut, le programme est la boîte de dialogue
                              d'impression graphique de KDE :

                                kprinter

                              Cette commande peut être modifiée en
                              définissant la valeur "altprintcommand" dans le
                              fichier de configuration de Tux Paint.

                              Voir la documentation sur "Options".

                      Réglages pour l'impression

                              (Windows et macOS)

                              Par défaut, Tux Paint imprime simplement sur
                              l'imprimante par défaut avec les paramètres par
                              défaut lorsque le bouton «Imprimer» est
                              enfoncé.

                              Cependant, si vous maintenez la touche [Alt]
                              (ou [Option]) du clavier tout en appuyant sur
                              le bouton "Imprimer, et ceci tant que vous
                              n'êtes pas en mode plein écran, la boîte de
                              dialogue de l'imprimante de votre système
                              d'exploitation apparaît, et vous pouvez
                              modifier les réglages.

                              Vous pouvez stocker les changements de
                              configuration, entre les sessions de Tux Paint,
                              en paramétrant l'option "printcfg".

                              Si l'option "printcfg" est utilisée, les
                              réglages d'impression seront chargés à partir
                              du fichier "printcfg.cfg" de votre répertoire
                              personnel (voir ci-dessous). Tout changement y
                              sera ernregistré.

                              Voir la documentation sur "Options".

                      Options de la boîte de dialogue de l'imprimante

                              Par défaut, Tux Paint affiche uniquement la
                              boîte de dialogue de l'imprimante (ou, sous
                              Linux / Unix, exécute "altprintcommand", par
                              exemple, "kprinter" au lieu de "lpr") si la
                              touche [Alt] (ou [Option] ) est maintenue
                              pendant en cliquant sur le bouton «Imprimer».

                              Cependant, ce comportement peut être modifié.
                              Vous pouvez toujours faire apparaître la boîte
                              de dialogue de l'imprimante en utilisant
                              "--altprintalways" sur la ligne de commande ou
                              "altprint=always" dans le fichier de
                              configuration de Tux Paint. Inversement vous
                              pouvez empêcher la touche [Alt] / [Option]
                              d'avoir un effet en utilisant "--altprintnever"
                              ou "altprint=never".

                              Voir la documentation sur "Options".

     ----------------------------------------------------------------------

         Commande "Diapos" (sous "Ouvrir")

                 Le bouton "Diapositives" est disponible dans la boîte de
                 dialogue "Ouvrir". Il peut être utilisé pour lire une simple
                 animation dans Tux Paint, ou un diaporama. Il peut également
                 exporter un GIF animé basé sur les images choisies.

                      Choisir des images

                              Lorsque vous entrez dans la section "Diapos" de
                              Tux Paint, il affiche une liste de vos fichiers
                              enregistrés, tout comme la boîte de dialogue
                              "Ouvrir".

                              Cliquez sur chacune des images que vous
                              souhaitez afficher dans une présentation de
                              style diaporama, une par une. Un chiffre
                              apparaîtra sur chaque image, vous indiquant
                              dans quel ordre elles seront affichées.

                              Vous pouvez cliquer sur une image sélectionnée
                              pour la désélectionner (la retirer de votre
                              diaporama). Cliquez à nouveau dessus si vous
                              souhaitez l'ajouter à la fin de la liste.

                      Régler la vitesse de lecture

                              Une échelle mobile en bas à gauche de l'écran
                              (à côté du bouton "Lecture") peut être utilisée
                              pour régler la vitesse du diaporama ou du GIF
                              animé, du plus lent au plus rapide. Choisissez
                              le paramètre le plus à gauche pour désactiver
                              l'avancement automatique pendant la lecture
                              dans Tux Paint - vous devrez appuyer sur une
                              touche ou cliquer pour passer à la diapositive
                              suivante (voir ci-dessous).

                              Remarque : le paramètre le plus lent ne fait
                              pas automatiquement avancer les diapositives.
                              Utilisez-le lorsque vous souhaitez les
                              parcourir manuellement. (Cela ne s'applique pas
                              à un GIF animé exporté.)

                      Lecture dans Tux Paint

                              Pour lire un diaporama dans Tux Paint, cliquez
                              sur le bouton "Départ". (Remarque : si vous
                              n'avez sélectionné AUCUNE image, TOUTES vos
                              images enregistrées seront lues dans le
                              diaporama !)

                              Pendant le diaporama, appuyez sur [Espace],
                              [Entrée] ou [Retour] , ou sur [Flèche droite] -
                              ou cliquez sur le bouton "Suivant" en bas à
                              gauche - pour passer manuellement à la
                              diapositive suivante. Appuyez sur [Flèche
                              gauche] pour revenir à la diapositive
                              précédente.

                              Appuyez sur [Escape] , ou cliquez sur le bouton
                              "Retour" en bas à droite, pour quitter le
                              diaporama et revenir à l'écran de sélection
                              d'image du diaporama.

                      Exporter un GIF animé

                              Cliquez sur le bouton "Exporter GIF" en bas à
                              droite pour que Tux Paint génère un fichier GIF
                              animé basé sur les images sélectionnées.

                              Remarque : On doit sélectionner au moins deux
                              images. (Pour exporter une seule image,
                              utilisez l'option "Exporter" de la boîte de
                              dialogue "Ouvrir".) Si aucune image n'est
                              sélectionnée, Tux Paint n'essaiera PAS de
                              générer un GIF basé sur toutes les images
                              enregistrées.

                              Appuyer sur [Escape] pendant l'exportation
                              annulera le processus et vous ramènera à la
                              boîte de dialogue "Diaporama".

                 Cliquez sur "Retour" dans l'écran de sélection d'image du
                 diaporama pour revenir à la boîte de dialogue "Ouvrir".

     ----------------------------------------------------------------------

         Commande "Quitter"

                 Cliquez sur le bouton "Quitter", fermez la fenêtre de Tux
                 Paint ou appuyez sur la touche [Escape] pour quitter Tux
                 Paint.

                 On vous demandera d'abord si vous voulez vraiment arrêter.

                 Si vous choisissez de quitter et que vous n'avez pas
                 enregistré l'image actuelle, il vous sera d'abord demandé si
                 vous souhaitez l'enregistrer. S'il ne s'agit pas d'une
                 nouvelle image, il vous sera alors demandé si vous souhaitez
                 enregistrer sur l'ancienne version ou créer une nouvelle
                 entrée. (Voir Sauvegarder ci-dessus.)

                 Remarque :: Si l'image est enregistrée, elle sera rechargée
                 automatiquement la prochaine fois que vous exécuterez Tux
                 Paint -- à moins que l'option"startblank" ait été activée.

                 Remarque :: Le bouton "Quitter" et la touche [Escape]
                 peuvent être désactivés par le biais de option "noquit".

                 Dans ce cas, le bouton "Fermer la fenêtre" sur la barre de
                 titre de Tux Paint (si vous n'êtes pas en mode plein écran)
                 ou la touche [Alt] + [F4] peut être utilisée pour quitter.

                 Si aucune de ces options n'est possible, la séquence de
                 touches [Shift] + [Control] + [Escape] peut être utilisée
                 pour quitter.

                 Voir la documentation sur "Options".

     ----------------------------------------------------------------------

         Coupure du son

                 Il n'y a pas de bouton de commande à l'écran pour le moment,
                 mais en appuyant sur les touches [Alt] + [S] , les effets
                 sonores peuvent être désactivés et réactivés pendant que le
                 programme est en cours d'exécution.

                 Notez que si les sons sont complètement désactivés via
                 l'option "nosound, la combinaison des touches [Alt] + [S]
                 n'a pas d'effet (c'est-à-dire qu'il ne peut pas être utilisé
                 pour activer les sons lorsque le parent / enseignant veut
                 qu'ils soient désactivés.)

     ----------------------------------------------------------------------

                   Chargement d'autres images dans Tux Paint

     Étant donné que la boîte de dialogue «Ouvrir» de Tux Paint n'affiche que
     les images que vous avez créées avec Tux Paint, que se passe-t-il si
     vous souhaitez charger une autre image ou photo dans Tux Paint pour les
     éditer et dessiner par-dessus ?

     Pour ce faire, il vous suffit de convertir l'image dans le format
     utilisé par Tux Paint, qui est —PNG (Portable Network Graphic), et de la
     placer dans le répertoire "saved" de Tux Paint. C'est là où on les
     trouve (par défaut) :

     Windows 10, 8, 7, Vista
             Dans le dossier utilisateurs "AppData", par ex. "C:\Users\nom
             d'utilisateur\AppData\Roaming\TuxPaint\saved\".

     Windows 2000, XP
             Dans le dossier utilisateurs "Application Data", par ex.
             "C:\Documents and Settings\nom d'utilisateur\Application
             Data\TuxPaint\saved\".

     macOS
             Dans le dossier utilisateurs "Library", par ex. "/Users/nom
             d'utilisateur/Library/Application Support/Tux Paint/saved/".

     Linux/Unix
             Dans un répertoire caché ".tuxpaint" qui est dans le répertoire
             personnel de l'utilisateur ("$HOME"), par ex. "/home/nom
             d'utilisateur/.tuxpaint/saved/".

     Remarque : C'est également à partir de ce dossier que vous pouvez copier
     ou ouvrir des images dessinées dans Tux Paint à l'aide d'autres
     applications, ainsi l'option 'Export' de la boîte de dialogue 'Ouvrir'
     de Tux Paint peut être utilisée pour les copier dans un endroit plus
     facile d'accès plus sûr.

Utilisation du script d'importation "tuxpaint-import"

       Les utilisateurs Linux et Unix peuvent utiliser script shell
       "tuxpaint-import" qui s'installe lorsque vous installez Tux Paint. Il
       utilise certains outils NetPBM pour convertir l'image ("anytopnm"), la
       redimensionner pour qu'elle tienne dans le canevas de Tux Paint
       ("pnmscale") et la convertir en PNG ("pnmtopng").

       Il utilise également la commande "date" pour obtenir l'heure et la
       date actuelles, qui sont la convention de dénomination des fichiers
       utilisée par Tux Paint pour les fichiers enregistrés. (N'oubliez pas
       que vous n'êtes jamais invité à entrer un "nom de fichier " lorsque
       vous allez enregistrer ou ouvrir des images !)

       Pour utiliser ce script, exécutez simplement la commande à partir
       d'une invite de ligne de commande et indiquez-lui le (s) nom (s) du ou
       des fichiers que vous voulez convertir.

       Ils seront convertis et placés dans votre répertoire "saved" de Tux
       Paint. (Remarque: si vous faites cela pour un autre utilisateur - par
       exemple, votre enfant, vous devrez vous assurer d'exécuter la commande
       sous son compte.)

       Exemple :

         $ tuxpaint-import grandma.jpg
         grandma.jpg -> /home/username/.tuxpaint/saved/20211231012359.png
         jpegtopnm: WRITING A PPM FILE

       La première ligne ("tuxpaint-import grandma.jpg") est la commande à
       exécuter. Les deux lignes suivantes sont les sorties du programme
       pendant qu'il fonctionne.

       Vous pouvez maintenant charger Tux Paint, et une version de cette
       image originale sera disponible dans la boîte de dialogue «Ouvrir».
       Double-cliquez simplement sur son icône !

Importer des images manuellement

       Les utilisateurs de Windows, macOS et Haiku désirant importer des
       images dans Tux Paint doivent le faire manuellement.

       Chargez un programme graphique capable à la fois de charger votre
       image et d'enregistrer un fichier au format PNG. (Voir le fichier de
       documentation "PNG.html" pour une liste des logiciels suggérés et
       d'autres références.)

       Lorsque Tux Paint charge une image qui n'a pas la même taille que son
       canevas de dessin, il met à l'échelle (et parfois en coloriant les
       bords) l'image pour qu'elle tienne dans le canevas.

       Pour éviter que l'image ne soit étirée ou maculée, vous pouvez la
       redimensionner à la taille de la toile de Tux Paint. Cette taille
       dépend de la taille de la fenêtre Tux Paint, ou de la résolution à
       laquelle Tux Paint est exécuté, si il est en plein écran. (Remarque :
       la résolution par défaut est de 800x600.) Voir "Calculer les
       dimensions des images" ci-dessous.

       Sauvegarder l'image au format PNG. Il est fortement recommandé que
       vous nommiez le fichier en utilisant la date et l'heure courante,
       puisque c'est ce que Tux Paint utilise :

         YYYYMMDDhhmmss.png

         * YYYY = Year
         * MM = Mois (deux chiffres, "01"-"12")
         * DD = Jour du mois (deux chiffres, "01"-"31")
         * HH = Heure (deux chiffres,au format 24h, "00"-"23")
         * mm = Minute (deux chiffres, "00"-"59")
         * ss = Secondes (deux chiffres, "00"-"59")

       Exemple: "20210731110500.png",pour le 31 juillet 2021 à 11:05 du
       matin.

       Mettez cd fichier PNG dans votre répertoire Tux Paint "saved". (Voir
       ci-dessus)

  Calculer les dimensions des images

         Cette partie de documentation doit être réécrite puisque la nouvelle
         option "buttonsize" a été ajoutée. Pour l'heure, essayer de dessiner
         et de sauvegarder l'image dans Tux Paint, et ensuite déterminer
         quelle taille (largeur et hauteur en pixels) elle possède, et
         essayez de l'adapter lors de la mise à l'échelle en l'important dans
         Tux Paint.

     ----------------------------------------------------------------------

                            Lectures complémentaires

     Les autres documents inclus avec Tux Paint (dans le répertoire "docs")
     incluent :
       * Documentation sur l'outil 'Magie' ("magic-docs")
         Documentation pour chacun des outils "Magic" actuellement installés.
       * AUTHORS.txt
         Liste des auteurs et contributeurs.
       * CHANGES.txt
         Résumé des changements entre chaque version de Tux Paint.
       * COPYING.txt
         Licencd de copie, la GNU General Public License (GPL)
       * INSTALL.html
         Instructions pour compiler et installer Tux Paint, le cas échéant.
       * EXTENDING.html
         Des instructions détaillées sur la création de pinceaux, de tampons,
         d'images de démarrage et de modèles; et l'ajout de polices; et créer
         un nouveau clavier virtuel et des méthodes de saisie.
       * OPTIONS.html
         Instructions détaillées sur les options en ligne de commande et les
         fichiers de configuration, pour ceux qui ne veulent pas utiliser
         l'outil Tux Paint Config.
       * PNG.html
         Remarques sur la création d'images bitmap au format PNG à utiliser
         dans Tux Paint.
       * SVG.html
         Remarques sur la création d'images vectorielles au format SVG à
         utiliser dans Tux Paint.
       * SIGNALS.html
         Informations sur les signaux POSIX auxquels répond Tux Paint.

     ----------------------------------------------------------------------

                           Comment obtenir de l'aide

     Si vous avez besoin d'aide, il existe de nombreux moyens d'interagir
     avec les développeurs de Tux Paint et les autres utilisateurs.

       * Mentionner des bogues, ou demander de nouvelles fonctionnalités via
         le système de suivi des bogues
       * Participer aux nombreuses listes de diffusion de Tux Paint
       * Chatter avec les développeurs et d'autres utilisateurs via IRC
       * Contacter les développeurs directement

     Pour en savoir plus, visitez la page "Contact" du site officiel de Tux
     Paint : http://tuxpaint.org/contact/

     ----------------------------------------------------------------------

                               Comment participer

     Tux Paint est un projet mené par des volontaires, et nous serions
     heureux d'accepter votre aide dans des tas de domaines.

       * Traduire Tux Paint dans une autre langue
       * Améliorer les traductions existantes
       * Créer des oeuvres (tampons, images de démarrage, modèles, pinceaux)
       * Ajouter ou améliorer des caractéristiques ou bien des outils "Magie"
       * Créer un programme d'étude en classe
       * Promouvoir ou aider ceux qui utilisent Tux Paint

     Pour en savoir plus, visitez la page "Nous aider" du site officiel de
     Tux Paint : http://tuxpaint.org/help/
