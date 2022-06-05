                                   Tux Paint
                                 version 0.9.28

                      Un programme simple pour les enfants

        Copyright © 2002-2022 by divers contributeurs; see AUTHORS.txt.
                             https://tuxpaint.org/
                           @TuxPaintTweets on Twitter
                              Tux Paint on Tumblr

                                  juin 4, 2022

          +----------------------------------------------------------+
          |Table des matières                                        |
          |----------------------------------------------------------|
          |  * À propos de Tux Paint                                 |
          |  * Utiliser Tux Paint                                    |
          |       * Lancement de Tux Paint                           |
          |       * Écran titre                                      |
          |       * Écran principal                                  |
          |       * Outils disponibles                               |
          |            * Outils de dessin                            |
          |                 * Outil "Peinture" (pinceau)             |
          |                 * Outil "Tampon" (tampon de caoutchouc)  |
          |                 * Outil "Lignes"                         |
          |                 * Outil "Formes"                         |
          |                 * Outils "Texte" et "Étiquette"          |
          |                 * Outil "Remplir"                        |
          |                 * Outil "Magie" (Effets spéciaux)        |
          |                 * Outil "Gomme"                          |
          |            * Autres contrôles                            |
          |                 * "Undo" and "Redo" Commands             |
          |                 * Commande "Nouveau"                     |
          |                 * Commande "Ouvrir"                      |
          |                 * Commande "Sauvegarder"                 |
          |                 * Commande "Imprimer"                    |
          |                 * Commande "Diapos" (sous "Ouvrir")      |
          |                 * Commande "Quitter"                     |
          |                 * Coupure du son                         |
          |  * Chargement d'autres images dans Tux Paint             |
          |  * Lectures complémentaires                              |
          |  * Comment obtenir de l'aide                             |
          |  * Comment participer                                    |
          +----------------------------------------------------------+

                             À propos de Tux Paint

Qu'est-ce que "Tux Paint" ?

   Tux Paint est un programme de dessin gratuit conçu pour les jeunes enfants
   (enfants de 3 ans et plus). Il possède une interface simple et facile à
   utiliser, des effets sonores amusants et une mascotte de dessin animé pour
   encourager et guider les enfants lorsqu'ils utilisent le programme. Sont
   fournis une toile vierge et un ensemble d'outils de dessin pour aider
   votre enfant à être créatif.

Licence

   Tux Paint est un projet Open Source, un logiciel libre publié sous la
   licence publique générale GNU (GPL). Il est gratuit et le «code source» du
   programme est disponible. (Cela permet à d'autres d'ajouter des
   fonctionnalités, de corriger des bogues et d'utiliser des parties du
   programme dans leur propre logiciel sous GPL.)

   Voir COPYING.txt pour le texte complet de la licence GPL.

Objectives

   Facile et amusant
           Tux Paint se veut un simple programme de dessin pour les jeunes
           enfants. Il ne s'agit pas d'un programme de dessin à usage
           général. Il se veut amusant et facile à utiliser. Les effets
           sonores et un personnage de dessin animé permettent à
           l'utilisateur de savoir ce qui se passe tout en le divertissant.
           Il existe également des pointeurs de souris, de style dessin
           animé, extra-larges.

   Flexibilité
           Tux Paint est extensible. Les brosses et les formes de "tampon en
           caoutchouc" peuvent être déposées et retirées. Par exemple, un
           enseignant peut apporter une collection de formes animales et
           demander à ses élèves de dessiner un écosystème. Chaque forme peut
           avoir un son qui l'accompagne et un texte est affiché lorsque
           l'enfant sélectionne la forme.

   Portabilité
           Tux Paint est portable sur diverses plates-formes informatiques:
           Windows, Macintosh, Linux, etc. L'interface est la même sur
           toutes. Tux Paint fonctionne correctement sur les systèmes plus
           anciens (comme un Pentium 133), et peut être modifié pour mieux
           fonctionner sur des systèmes lents.

   Simplicité
           Il n'y a pas d'accès direct à la complexité sous-jacente de
           l'ordinateur. L'image en cours est conservée lorsque le programme
           se ferme et réapparaît au redémarrage. L'enregistrement d'images
           ne nécessite pas la création de noms de fichiers ou l'utilisation
           du clavier. L'ouverture d'une image se fait en la sélectionnant
           dans une collection de vignettes. L'accès à d'autres fichiers sur
           l'ordinateur est restreint.

                               Utiliser Tux Paint

Lancement de Tux Paint

  Utilisateurs de Linux/Unix

   Tux Paint should have placed a launcher icon in your KDE and/or GNOME
   menus, under 'Graphics.'

   Autrement, vous pouvez exécuter la commande suivante à une invite du shell
   (par exemple, "$") :

     $ tuxpaint

   Si des erreurs se produisent, elles seront affichées sur le terminal
   ("stderr").

  Utilisateurs de Windows

                                                      [Icône pour Tux Paint]  
                                                            Tux Paint         

   Si vous avez installé Tux Paint sur votre ordinateur en utilisant le 'Tux
   Paint Installer', il vous aura demandé si vous vouliez un raccourci de
   menu 'Démarrer' et / ou un raccourci sur le bureau. Si vous avez accepté,
   vous pouvez simplement exécuter Tux Paint à partir de la section 'Tux
   Paint' de votre menu 'Démarrer' (par exemple, sous «Tous les programmes»
   sous Windows XP), ou en double-cliquant sur l'icône «Tux Paint» sur votre
   bureau .

   Si vous avez installé Tux Paint en utilisant le téléchargement 'ZIP-file',
   ou si vous avez utilisé 'Tux Paint Installer', mais que vous avez choisi
   de ne pas installer de raccourcis, vous devrez double-cliquer sur l'icône
   "tuxpaint.exe" dans le dossier "Tux Paint" de votre ordinateur.

   Par défaut, le 'Tux Paint Installer' mettra le dossier de Tux Paint dans
   "C:\Program Files\",ou bien vous avez pu changer cela lors de l'exécution
   du programme d'installation.

   Si vous avez utilisé le téléchargement de 'ZIP-file', le dossier de Tux
   Paint sera là où vous l'avez placé lorsque vous avez décompressé le
   fichier ZIP.

  Utilisateurs de macOS

   Double-cliquez simplement sur l'icône "Tux Paint".

   [Écran-titre]

Écran titre

   Lors du premier chargement de Tux Paint, un écran avec titre et crédits
   apparaîtra.

   Une fois le chargement terminé, appuyez sur une touche ou cliquez sur la
   souris pour continuer. (Ou, après environ 30 secondes, l'écran-titre
   disparaîtra automatiquement.)

Écran principal

   L'écran principal est divisé en plusieurs sections :

   [Outils : Peindre, Tampon, Lignes, Formes, Texte, Magie, Étiquette,
   Défaire, Refaire, Gomme, Nouveau, Ouvrir, Sauvegarder, Imprimer, Quitter]

   Côté gauche : la barre d'outils

           La barre d'outils contient les commandes pour dessiner et éditer.

           [Toile]

   Milieu : Toile pour Dessiner

           La plus grande partie de l'écran, au centre, est la toile de
           dessin. C'est évidemment là que vous dessinerez !

           💡 Remarque: la taille de la toile de dessin dépend de la taille de
           Tux Paint. Vous pouvez modifier la taille de Tux Paint à l'aide de
           l'outil de configuration tuxpaint-config, ou par d’autres moyens.
           Consultez la documentation OPTIONS pour plus de détails.

           [Sélecteurs - Pinceaux, Lettres, Formes, Tampons]

   Côté droit : sélecteur

           En fonction de l'outil en cours d'utilisation, le sélecteur
           affiche différentes choses. Par exemple, lorsque l'outil Pinceau
           est sélectionné, il affiche les différents pinceaux disponibles.
           Lorsque l'outil Tampon en caoutchouc est sélectionné, il affiche
           les différentes formes que vous pouvez utiliser.

           [Couleurs - Noir, Blanc, Rouge, Rose, Orange, Jaune, Vert, Cyan,
           Bleu, Pourpre, Brun, Gris]

   En bas : couleurs

           When the active tool supports colors, a palette of colors choices
           will be shown near the bottom of the screen. Click one to choose a
           color, and it will be used by the active tool. (For example, the
           "Paint" tool will use it as the color to draw with the chosen
           brush, and the "Fill" tool will use it as the color to use when
           flood-filling an area of the picture.)

           On the far right are three special color options:
              * Color Picker
                The "color picker" (which has an outline of an eye-dropper)
                allows you to pick a color found within your drawing.
                (A shortcut key is available to access this feature quickly;
                see below.)
              * Rainbow Palette
                The rainbow palette allows you to pick any color by choosing
                the hue, saturation, and value of the color you want. A box
                on the left displays hundreds of hues — from red at the top
                through to violet at the bottom — at hundreds of
                saturation/intensity levels — from pale & washed-out on the
                left through to pure on the right. A grey vertical bar
                provides access to hundreds of value levels — from lighest at
                the top through to darkest at the bottom.
                Click the green checkbox button to select the color, or the
                "Back" button to dismiss the pop-up without picking a new
                color.
              * Color Mixer
                The "color mixer" (which has silhouette of a paint palette)
                allows you to create colors by blending primary additive
                colors — red, yellow, and blue — along with white (to
                "tint"), grey (to "tone"), and black (to "shade").
                You may click any button multiple times (for example, red +
                red + yellow results in a red-orange color). The ratios of
                colors added are shown at the bottom.
                You can start over (reset to no colors in your picture) by
                clicking the "Clear" button. You can also undo or redo
                multiple steps of mixing, in case you made a mistake (without
                having to start over).
                Click the green checkbox button to select the color, or the
                "Back" button to dismiss the pop-up without picking a new
                color.

           ⌨ When the active tool supports colors, a shortcut may be used to
           access the "color picker" option more quickly. Hold the [Control]
           key while clicking, and the color under the mouse cursor will be
           shown at the bottom. You may drag around to canvas to find the
           color you want. When you release the mouse button, the color under
           the cursor will be selected. If you release the mouse outside of
           the canvas (e.g., over the "Tools" area), the color selection will
           be left unchanged. (This is similar to clicking the"Back" button
           that's available when bringing up the "color picker" option via
           its button the color palette.)

           ⚙ Note: You can define your own colors for Tux Paint. See the
           "Options" documentation.

           (Par exemple : Choisis une forme. Clique pour démarrer le dessin ,
           fais glisser et continue jusqu'à la taille désirée. Déplace-toi
           pour la faire tourner, et clique pour dessiner.)

   En bas : zone d'aide

           Tout en bas de l'écran, Tux, le pingouin Linux, fournit des
           conseils et d'autres informations pendant que vous dessinez.

Outils disponibles

  Outils de dessin

   Outil "Peinture" (pinceau)

           L'outil Pinceau vous permet de dessiner à main levée, en utilisant
           différents pinceaux (choisis dans le sélecteur à droite) et
           couleurs (choisis dans la palette de couleurs qui est en bas).

           Si vous maintenez le bouton de la souris enfoncé et déplacez la
           souris, elle dessine au fur et à mesure que vous vous déplacez.

           Cerains pinceaux sont animés — ils changent de forme lorsque vous
           dessinez. Un bon exemple est le pinceau 'grappe' livré avec Tux
           Paint. Ces pinceaux ont une icône avec une petite pellicule
           dessinée sur le bouton de sélection.

           D'autres pinceaux sont directionnels — leur forme dépend de la
           direction dans laquelle vous dessinez. Un exemple est le pinceau
           flèche livré avec Tux Paint. Ces pinceaux ont une petite étoile à
           8 branches sur leur icône du bouton de sékection.

           Et enfin, certains pinceaux peuvent être = la fois directionnels
           et animés. Des exemples sont les pinceaux 'chat' et 'écureuil'
           livrés avec Tux Paint. Ces pinceaux ont à la fois une pellicule et
           une étoile à 8 branches sur leur icône.

           Pendant que vous dessinez, un son est joué. Plus la brosse est
           grosse, plus le son est grave.

           Espacement des pinceaux

             The space between each position where a brush is applied to the
             canvas can vary. Some brushes (such as the footprints and
             flower) are spaced, by default, far enough apart that they don't
             overlap. Other brushes (such as the basic circular ones) are
             spaced closely, so they make a continuous stroke.

             The default spacing of brushes may be overridden using by
             clicking within the triangular-shaped series of bars at the
             bottom right; the larger the bar, the wider the spacing. Brush
             spacing affects both tools that use the brushes: the "Paint"
             tool and the "Lines" tool.

             ⚙ Note: If the "nobrushspacing" option is set, Tux Paint won't
             display the brush spacing controls. See the "Options"
             documentation.

   Outil "Tampon" (tampon de caoutchouc)

           L'outil Tampon montre un ensemble de tampons en caoutchouc ou
           d'autocollants. Il vous permet de coller des images pré-dessinées
           ou photographiques (comme l'image d'un cheval, d'un arbre ou de la
           lune) dans votre dessin.

           Lorsque vous déplacez la souris sur le canevas, une forme suit la
           souris, indiquant où le tampon sera placé, ainsi que sa taille.

           Il peut y avoir de nombreuses catégories de timbres (par ex.
           animaux, plantes, espace extra-atmosphérique, véhicules,
           personnes, etc.). Utilisez les flèches gauche et droite pour
           parcourir les différentes collections.

           Avant de `` tamponner '' une image sur votre dessin, différents
           effets peuvent parfois être appliqués (en fonction du tampon) :

              * Certains tampons peuvent être colorés ou teintés. Si la
                palette de couleurs sous le canevas est activée, vous pouvez
                cliquer sur les couleurs pour changer la teinte ou la couleur
                du tampon avant de le placer dans le dessin.
              * Les tampons peuvent être rétrécis et agrandis, en cliquant
                dans l'ensemble de barres (de forme triangulaire) en bas à
                droite; plus la barre est grande, plus le tampon apparaîtra
                grand sur votre dessin.
              * De nombreux tampons peuvent être retournés verticalement ou
                affichés sous forme d'image miroir à l'aide des boutons de
                commande en bas à droite.

           Les tampons peuvent avoir un effet sonore et / ou une description
           orale (parlés). Les boutons en bas à gauche (près de Tux, le
           pingouin Linux) vous permettent de rejouer les effets sonores et
           la description du tampon actuellement sélectionné.

           ⚙ Note: If the "nostampcontrols" option is set, Tux Paint won't
           display the Mirror, Flip, Shrink and Grow controls for stamps. See
           the "Options" documentation.

   Outil "Lignes"

           Cet outil vous permet de dessiner des lignes droites à l'aide des
           différents pinceaux et couleurs que vous utilisez habituellement.

           Click the mouse and hold it to choose the starting point of the
           line. As you move the mouse around, a thin 'rubber-band' line will
           show where the line will be drawn. At the bottom, you'll see the
           angle of your line, in degrees. A line going straight to the right
           is 0°, a line going straight up is 90°, a line going straight left
           is 180°, a line going straight down is 270°, and so on.

           Relâchez la souris pour terminer la ligne. On entend alors le son
           "sproing !".

           Certains pinceaux sont animés, et montre un ensemble formes le
           long d'une ligne. D'autres sont directionnels et montre des formes
           différentes suivant l'angle du pinceau. Et enfin certains sont à
           la fois animés et directionnels. Voir "Paint", au-dessus, pour en
           savoir plus.

           Different brushes have different spacing, leaving either a series
           of individual shapes, or a continuous stroke of the brush shape.
           Brush spacing may be adjusted. See "Paint", above, to learn more.

   Outil "Formes"

           Cet outil vous permet de dessiner des formes simples remplies ou
           non remplies.

           Choisissez une forme dans le sélecteur de droite (cercle,
           carré,ovale, etc.).

           Utilisez les options en bas à droite pour choisir le comportement
           de l'outil :

                Formes à partir du centre
                        The shape will expand from where you initially
                        clicked, and will be centered around that position.

                        📜 This was Tux Paint's only behavior through version
                        0.9.24.)

                Formes à partir d'un coin
                        The shape will extend with one corner starting from
                        where you initially clicked. This is the default
                        method of most other traditional drawing software.

                        📜 This option was added starting with Tux Paint
                        version 0.9.25.)

           ⚙ Remarque : si les contrôles de forme sont désactivés (par
           exemple, avec l'option "noshapecontrols"), il n'y aura pas de
           contrôle et la méthode "formes à partir du centre" sera utilisée.

           Dans le dessin, cliquez sur la souris et maintenez-la pour étirer
           la forme à partir de l'endroit où vous avez cliqué. Certaines
           formes peuvent changer de proportion (par exemple, le rectangle et
           l'ovale peuvent être plus larges que hauts ou plus hauts que
           larges), d'autres pas (par exemple, carré et cercle).

           For shapes that can change proportion, the aspect ratio of the
           shape will be shown at the bottom. For example: "1:1" will be
           shown if it is "square" (as tall as it is wide); "2:1" if it is
           either twice as wide as it is tall, or twice as tall as it is
           wide; and so on.

           Relâchez la souris lorsque vous avez terminé l'étirement.

                Mode normal

                        Now you can move the mouse around the canvas to
                        rotate the shape. The angle your shape is rotated
                        will be shown at the bottom, in degrees (similar to
                        the "Lines" tool, described above).

                        Cliquez à nouveau sur le bouton de la souris et la
                        forme sera dessinée avec la couleur en cours.

                Mode de Formes Simples
                        If the "simple shapes" option is enabled, the shape
                        will be drawn on the canvas when you let go of the
                        mouse button. (There's no rotation step.)

                        ⚙ See the "Options" documentation to learn about the
                        "simple shapes" ("simpleshapes") option.

   Outils "Texte" et "Étiquette"

           Choose a font (from the 'Letters' available on the right) and a
           color (from the color palette near the bottom). You may also apply
           a bold, and/or an italic styling effect to the text. Click on the
           screen and a cursor will appear. Type text and it will show up on
           the screen. (You can change the font, color, and styling while
           entering the text, before it is applied to the canvas.)

           Appuyez sur [Entrée] ou [Retour] et le texte sera inclus dans
           l'image et le curseur se déplacera d'une ligne vers le bas.

           Sinon, appuyez sur [Tab] et le texte sera inclus dans l'image,
           mais le curseur se déplacera vers la droite du texte, plutôt que
           vers le bas d'une ligne et vers la gauche. (Cela peut être utile
           pour créer une ligne de texte avec des couleurs, des polices, des
           styles et des tailles variés.)

           Cliquer ailleurs dans l'image alors que l'entrée de texte est
           toujours active entraîne le déplacement de la ligne de texte
           actuelle vers cet emplacement (et vous pouvez continuer à la
           modifier).

                "Texte" par rapport à "Étiquette"

                        L' outil Texte est l'outil de saisie de texte
                        original de Tux Paint. Le texte saisi à l'aide de cet
                        outil ne peut pas être modifié ou déplacé
                        ultérieurement, car il fait partie du dessin.
                        Cependant, comme le texte fait partie de l'image, il
                        peut être dessiné ou modifié à l'aide des effets de
                        l'outil Magie (par exemple, taché, teinté, gaufré,
                        etc.)

                        Lors de l'utilisation de l' outil Étiquette (qui a
                        été ajouté à Tux Paint dans la version 0.9.22), le
                        texte `` flotte '' sur l'image, et les détails de
                        l'étiquette (le texte, la position de l'étiquette, le
                        choix de la police et la couleur ) sont stockés
                        séparément. Cela permet à l'étiquette d'être
                        repositionnée ou modifiée ultérieurement.

                        To edit a label, click the label selection button.
                        All labels in the drawing will appear highlighted.
                        Click one — or use the [Tab] key to cycle through all
                        the labels, and the [Entrée] or [Retour] key to
                        select one — and you may then edit the label. (Use
                        they [Backspace] key to erase characters, and other
                        keys to add text to the label; click in the canvas to
                        reposition the label; click in the palette to change
                        the color of the text in the label; etc.)

                        You may "apply" a label to the canvas, painting the
                        text into the picture as if it had been added using
                        the Text tool, by clicking the label application
                        button. (This feature was added in Tux Paint version
                        0.9.28.) All labels in the drawing will appear
                        highlighted, and you select one just as you do when
                        selecting a label to edit. The chosen label will be
                        removed, and the text will be added directly to the
                        canvas.

                        ⚙ L' outil Étiquette peut être désactivé (par
                        exemple, en sélectionnant "Désactiver l'outil
                        'Label'" dans Tux Paint Config ou bien en exécutant
                        Tux Paint en ligne de commande avec l'option
                        "nolabel").

                Saisie de caractères internationaux

                        Tux Paint permet de saisir des caractères dans
                        différentes langues. La plupart des caractères latins
                        ( A - Z , ñ , è , etc...) peuvent être saisis
                        directement. Certaines langues exigent que Tux Paint
                        soit commuté dans un mode d'entrée alternatif avant
                        la saisie, et certains caractères doivent être
                        composés en utilisant plusieurs touches.

                        Lorsque les paramètres régionaux de Tux Paint sont
                        définis sur l'une des langues fournissant des modes
                        de saisie alternatifs, une touche est utilisée pour
                        parcourir le ou les modes soit normaux (caractère
                        latin) soit spécifiques aux paramètres régionaux.

                        Currently supported locales, the input methods
                        available, and the key to toggle or cycle modes, are
                        listed below.

                           * Japanese — Romanized Hiragana and Romanized
                             Katakana — touche [Alt] droite or touche [Alt]
                             gauche
                           * Korean — Hangul 2-Bul — touche [Alt] droite or
                             touche [Alt] gauche
                           * Chinois traditionnel — touche [Alt] droite or
                             touche [Alt] gauche
                           * Thai — touche [Alt] droite

                        💡 Note: Many fonts do not include all characters for
                        all languages, so sometimes you'll need to change
                        fonts to see the characters you're trying to type.

                Clavier virtuel sur écran

                        An optional on-screen keyboard is available for the
                        Text and Label tools, which can provide a variety of
                        layouts and character composition (e.g., composing
                        "a" and "e" into "æ").

                        ⚙ See the "Options" and "Extending Tux Paint"
                        documentation for more information.

   Outil "Remplir"

           L'outil «Remplir» «remplit» une zone contiguë de votre dessin avec
           une couleur unie de votre choix. Trois options de remplissage sont
           offertes :
              * Solide — cliquez une fois pour remplir une zone avec une
                couleur unie.
              * Brush — click and drag to fill an area with a solid color
                using freehand painting.
              * Linéaire—cliquez et faites glisser pour remplir une zone avec
                une couleur qui s'atténue au fur et à mesure dans la
                direction où vous déplacez la souris.
              * Radial—cliquez une fois pour remplir une zone avec une
                couleur qui s'atténue graduellement, à partir de l'endroit où
                vous avez cliqué.

           📜 Note: Prior to Tux Paint 0.9.24, "Fill" was a Magic tool (see
           below). Prior to Tux Paint 0.9.26, the "Fill" tool only offered
           the 'Solid' method of filling.

   Outil "Magie" (Effets spéciaux)

           L'outil «Magie» est en fait un ensemble d'outils spéciaux.
           Sélectionnez l'un des effets «magiques» dans le sélecteur de
           droite. Ensuite, selon l'outil, vous pouvez soit cliquer et faire
           glisser dans l'image, et / ou simplement cliquer une fois sur
           l'image pour appliquer l'effet.

           Si l'outil peut être utilisé en cliquant et en faisant glisser, un
           bouton «peinture» sera disponible sur la gauche, sous la liste des
           outils «magiques» sur le côté droit de l'écran. Si l'outil peut
           affecter toute l'image en entier, un bouton «Image entière» sera
           disponible sur la droite.

           Voir les instructions pour chaque outil 'Magie' (dans le dossier
           'magic-docs').

   Outil "Gomme"

           Cet outil est similaire au pinceau. Partout où vous cliquez (ou
           cliquez et faites glisser), l'image sera effacée. (Cela peut être
           du blanc, une autre couleur ou une image d'arrière-plan, selon
           l'image.)

           Un certain nombre de tailles de gommes sont disponibles, soit
           rondes soit carrées.

           Lorsque vous déplacez la souris, un contour carré suit le
           pointeur, indiquant quelle partie de l'image sera effacée en
           blanc.

           Au fur et à mesure que vous effacez, un grincement est émis.

  Autres contrôles

   "Undo" and "Redo" Commands

           Clicking the "Undo" button will undo (revert) the last drawing
           action. You can even undo more than once!

           ⌨ Remarque : vous pouvez également appuyer [Control / ⌘] + [Z] sur
           le clavier pour Défaire.

           Clicking the "Redo" button will redo the drawing action you just
           un-did via the "Undo" command.

           Tant que vous ne dessinez plus, vous pouvez refaire autant de fois
           que vous avez défait !

           ⌨ Remarque : vous pouvez également appuyer [Control / ⌘] + [R] sur
           le clavier pour Refaire.

   Commande "Nouveau"

           Cliquez sur le bouton "Nouveau" pour démarrer un nouveau dessin.
           Une boîte de dialogue apparaîtra, avec laquelle vous pouvez
           choisir de commencer une nouvelle image en utilisant une couleur
           d'arrière-plan unie, ou en utilisant une image 'Starter' ou
           'Template' (voir ci-dessous). On vous demandera d'abord si vous
           voulez vraiment faire cela.

           ⌨ Remarque : vous pouvez également appuyer [Control / ⌘] + [N] sur
           le clavier pour commencer un nouveau dessin.

           Special Solid Background Color Choices

             Along with the preset solid colors, you can also choose colors
             using a rainbow palette or a "color mixer". These operate
             identically to the options found in the color palette shown
             below the canvas when drawing a picture. See Main Screen >
             Lower: Colors > Special color options for details.

           Images de "Démarrage" et images "Modèle"

             Les "Images de démarrage" se comportent comme une page d'un
             livre de coloriage - un contour noir et blanc d'une image, que
             vous pouvez ensuite colorier, et le contour noir reste intact -
             ou comme une photographie 3D, où vous dessinez entre une couche
             de premier plan et une d'arrière-plan.

             Les "Images modèle" sont semblables, mais fournissent simplement
             un dessin d'arrière-plan sur lequel travailler. Contrairement
             aux «Images de démarrage», rien de ce que vous dessinerez ne
             restera au premier plan.

             Lorsque vous utiliserez l'outil «Gomme», l'image d'origine du
             «Démarrage» ou du «Modèle» réapparaîtra. Les outils magiques
             "Retourner" et "Miroir" affecteront aussi bien l'orientation de
             "l'image de démarrage" que celle de "l'image Modèle".

             Lorsque vous chargez une 'Image modèle' ou 'image modèle',
             dessinez dessus, puis cliquez sur 'Sauvegarder', cela crée un
             nouveau fichier image - il n'écrase pas l'original, vous pouvez
             donc l'utiliser à nouveau plus tard (en y accédant depuis la
             boîte de dialogue 'Nouveau').

   Commande "Ouvrir"

           Cela vous montre une liste de toutes les images que vous avez
           enregistrées. S'il y en a plus que ce que peut contenir l'écran,
           utilisez les flèches «Haut» et «Bas» en haut et en bas de la liste
           pour faire défiler la liste des images.

           Cliquez sur une image pour la sélectionner, puis ...
              * Cliquez sur le bouton vert "Ouvrir" en bas à gauche de la
                liste pour charger l'image sélectionnée.

                (Vous pouvez également double-cliquer sur l'icône d'une image
                pour la charger.)

                💡 If choose to open a picture, and your current drawing
                hasn't been saved, you will be prompted as to whether you
                want to save it or not. (See "Save," below.)

              * Cliquez sur le bouton marron "Effacer" (poubelle) en bas à
                droite de la liste pour effacer l'image sélectionnée. (Il
                vous sera demandé de confirmer.)

                📜 Note: On Linux (as of version 0.9.22) and Windows (as of
                version 0.9.27), the picture will be placed in your desktop's
                trash can / recycle bin (where you may recover and restore
                it, if you change your mind).

              * Cliquez sur le bouton "Exporter" près du coin inférieur droit
                pour exporter l'image vers votre dossier d'exportation. (par
                exemple, "~/Pictures/TuxPaint/")

           From the "Open" screen you can also:
              * Click the blue 'Slides' (slide projector) button at the lower
                left to go to slideshow mode. See "Slides", below, for
                details.

              * Ou cliquez sur le bouton fléché rouge «Retour» en bas à
                droite de la liste pour annuler et revenir à l'image que vous
                étiez en train de dessiner.

           ⌨ Remarque : vous pouvez également appuyer [Control / ⌘] + [O] sur
           le clavier pour afficher la boîte de dialogue "Ouvrir".

   Commande "Sauvegarder"

           Pour sauvegarder votre image en cours.

           Si vous ne l'avez pas enregistré auparavant, il créera une
           nouvelle entrée dans la liste des images enregistrées.
           (c'est-à-dire qu'il créera un nouveau fichier)

           💡 Remarque : il ne vous demandera rien (par exemple, un nom de
           fichier). Il enregistrera simplement l'image et fera le bruit d'un
           obturateur d'appareil photographique.

           Si vous avez déjà enregistré l'image, ou s'il s'agit d'une image
           que vous venez de charger à l'aide de la commande "Ouvrir", il
           vous sera d'abord demandé si vous voulez écraser l'ancienne
           version ou bien créer une nouvelle entrée (un nouveau fichier).

           ⚙ Remarque : si les options " saveover" ou " saveovernew" sont
           déjà définies, il ne sera rien demandé avant de sauvegarder. Voir
           la documentation Options.

           ⌨ Remarque : vous pouvez également appuyer [Control / ⌘] + [S] sur
           le clavier pour sauvegarde.

   Commande "Imprimer"

           Cliquez sur ce bouton et votre image sera imprimée !

           Sur la plupart des plates-formes, vous pouvez également maintenir
           la touche [Alt] (appelée [Option] sur Mac) tout en cliquant sur le
           bouton «Imprimer» pour obtenir une boîte de dialogue d'impression.
           Notez que cela pourrait ne pas fonctionner si vous exécutez Tux
           Paint en mode plein écran. Voir ci-dessous.

                Désactivation de l'impression

                        On peut définir une option "noprint", ce qui
                        entraînera la désactivation du bouton "Imprimer".

                        ⚙ Voir la documentation sur "Options".

                Restreindre l'impression

                        Si l'option "printdelay" a été utilisée, vous ne
                        pouvez imprimer — qu'une fois toutes les x secondes,
                        tel que vous l'avez défini.

                        Par exemple, avec "printdelay=60" dans le fichier de
                        configuration de Tux Paint, vous ne pouvez imprimer
                        qu'une fois par minute.

                        ⚙ Voir la documentation sur "Options".

                Commandes d'impression

                        (Linux et Unix uniquement)

                        Tux Paint imprime en générant une représentation
                        PostScript du dessin et en l'envoyant à un programme
                        externe. Par défaut, le programme est :

                          lpr

                        Cette commande peut être modifiée en définissant la
                        valeur "printcommand" dans le fichier de
                        configuration de Tux Paint.

                        Si la touche "[Alt]" du clavier est enfoncée tout en
                        cliquant sur le bouton «Imprimer», et tant que vous
                        n'êtes pas en mode plein écran, un programme
                        alternatif est exécuté. Par défaut, le programme est
                        la boîte de dialogue d'impression graphique de KDE :

                          kprinter

                        Cette commande peut être modifiée en définissant la
                        valeur "altprintcommand" dans le fichier de
                        configuration de Tux Paint.

                        ⚙ Voir la documentation sur "Options".

                Réglages pour l'impression

                        (Windows et macOS)

                        Par défaut, Tux Paint imprime simplement sur
                        l'imprimante par défaut avec les paramètres par
                        défaut lorsque le bouton «Imprimer» est enfoncé.

                        Cependant, si vous maintenez la touche [Alt] (ou
                        [Option]) du clavier tout en appuyant sur le bouton
                        "Imprimer, et ceci tant que vous n'êtes pas en mode
                        plein écran, la boîte de dialogue de l'imprimante de
                        votre système d'exploitation apparaît, et vous pouvez
                        modifier les réglages.

                        Vous pouvez stocker les changements de configuration,
                        entre les sessions de Tux Paint, en paramétrant
                        l'option "printcfg".

                        Si l'option "printcfg" est utilisée, les réglages
                        d'impression seront chargés à partir du fichier
                        "printcfg.cfg" de votre répertoire personnel (voir
                        ci-dessous). Tout changement y sera ernregistré.

                        ⚙ Voir la documentation sur "Options".

                Options de la boîte de dialogue de l'imprimante

                        Par défaut, Tux Paint affiche uniquement la boîte de
                        dialogue de l'imprimante (ou, sous Linux / Unix,
                        exécute "altprintcommand", par exemple, "kprinter" au
                        lieu de "lpr") si la touche [Alt] (ou [Option] ) est
                        maintenue pendant en cliquant sur le bouton
                        «Imprimer».

                        Cependant, ce comportement peut être modifié. Vous
                        pouvez toujours faire apparaître la boîte de dialogue
                        de l'imprimante en utilisant "--altprintalways" sur
                        la ligne de commande ou "altprint=always" dans le
                        fichier de configuration de Tux Paint. Inversement
                        vous pouvez empêcher la touche [Alt] / [Option]
                        d'avoir un effet en utilisant "--altprintnever" ou
                        "altprint=never".

                        ⚙ Voir la documentation sur "Options".

   Commande "Diapos" (sous "Ouvrir")

           Le bouton "Diapositives" est disponible dans la boîte de dialogue
           "Ouvrir". Il peut être utilisé pour lire une simple animation dans
           Tux Paint, ou un diaporama. Il peut également exporter un GIF
           animé basé sur les images choisies.

                Choisir des images

                        Lorsque vous entrez dans la section "Diapos" de Tux
                        Paint, il affiche une liste de vos fichiers
                        enregistrés, tout comme la boîte de dialogue
                        "Ouvrir".

                        Cliquez sur chacune des images que vous souhaitez
                        afficher dans une présentation de style diaporama,
                        une par une. Un chiffre apparaîtra sur chaque image,
                        vous indiquant dans quel ordre elles seront
                        affichées.

                        Vous pouvez cliquer sur une image sélectionnée pour
                        la désélectionner (la retirer de votre diaporama).
                        Cliquez à nouveau dessus si vous souhaitez l'ajouter
                        à la fin de la liste.

                Régler la vitesse de lecture

                        Une échelle mobile en bas à gauche de l'écran (à côté
                        du bouton "Lecture") peut être utilisée pour régler
                        la vitesse du diaporama ou du GIF animé, du plus lent
                        au plus rapide. Choisissez le paramètre le plus à
                        gauche pour désactiver l'avancement automatique
                        pendant la lecture dans Tux Paint - vous devrez
                        appuyer sur une touche ou cliquer pour passer à la
                        diapositive suivante (voir ci-dessous).

                        💡 Remarque : le paramètre le plus lent ne fait pas
                        automatiquement avancer les diapositives. Utilisez-le
                        lorsque vous souhaitez les parcourir manuellement.
                        (Cela ne s'applique pas à un GIF animé exporté.)

                Lecture dans Tux Paint

                        To play a slideshow within Tux Paint, click the
                        'Play' button.

                        💡 Note: If you hadn't selected any images, then all
                        of your saved images will be played in the slideshow!

                        Pendant le diaporama, appuyez sur [Espace], [Entrée]
                        ou [Retour] , ou sur [Flèche droite] - ou cliquez sur
                        le bouton "Suivant" en bas à gauche - pour passer
                        manuellement à la diapositive suivante. Appuyez sur
                        [Flèche gauche] pour revenir à la diapositive
                        précédente.

                        Appuyez sur [Escape] , ou cliquez sur le bouton
                        "Retour" en bas à droite, pour quitter le diaporama
                        et revenir à l'écran de sélection d'image du
                        diaporama.

                Exporter un GIF animé

                        Cliquez sur le bouton "Exporter GIF" en bas à droite
                        pour que Tux Paint génère un fichier GIF animé basé
                        sur les images sélectionnées.

                        💡 Note: At least two images must be selected. (To
                        export a single image, use the 'Export' option from
                        the main 'Open' dialog.) If no images are selected,
                        Tux Paint will not attempt to generate a GIF based on
                        all saved images.

                        Appuyer sur [Escape] pendant l'exportation annulera
                        le processus et vous ramènera à la boîte de dialogue
                        "Diaporama".

           Cliquez sur "Retour" dans l'écran de sélection d'image du
           diaporama pour revenir à la boîte de dialogue "Ouvrir".

   Commande "Quitter"

           Cliquez sur le bouton "Quitter", fermez la fenêtre de Tux Paint ou
           appuyez sur la touche [Escape] pour quitter Tux Paint.

           On vous demandera d'abord si vous voulez vraiment arrêter.

           If you choose to quit, and you haven't saved the current picture,
           you will first be asked if wish to save it. If it's not a new
           image, you will then be asked if you want to save over the old
           version, or create a new entry. (See "Save" above.)

           ⚙ Remarque :: Si l'image est enregistrée, elle sera rechargée
           automatiquement la prochaine fois que vous exécuterez Tux Paint --
           à moins que l'option"startblank" ait été activée.

           ⚙ Remarque :: Le bouton "Quitter" et la touche [Escape] peuvent
           être désactivés par le biais de option "noquit".

           Dans ce cas, le bouton "Fermer la fenêtre" sur la barre de titre
           de Tux Paint (si vous n'êtes pas en mode plein écran) ou la touche
           [Alt] + [F4] peut être utilisée pour quitter.

           Si aucune de ces options n'est possible, la séquence de touches
           [Shift] + [Control / ⌘] + [Escape] peut être utilisée pour
           quitter.

           ⚙ Voir la documentation sur "Options".

   Coupure du son

           Il n'y a pas de bouton de commande à l'écran pour le moment, mais
           en appuyant sur les touches [Alt] + [S] , les effets sonores
           peuvent être désactivés et réactivés pendant que le programme est
           en cours d'exécution.

           Notez que si les sons sont complètement désactivés via l'option
           "nosound, la combinaison des touches [Alt] + [S] n'a pas d'effet
           (c'est-à-dire qu'il ne peut pas être utilisé pour activer les sons
           lorsque le parent / enseignant veut qu'ils soient désactivés.)

           ⚙ Voir la documentation sur "Options".

                   Chargement d'autres images dans Tux Paint

   Étant donné que la boîte de dialogue «Ouvrir» de Tux Paint n'affiche que
   les images que vous avez créées avec Tux Paint, que se passe-t-il si vous
   souhaitez charger une autre image ou photo dans Tux Paint pour les éditer
   et dessiner par-dessus ?

   Pour ce faire, il vous suffit de convertir l'image dans le format utilisé
   par Tux Paint, qui est —PNG (Portable Network Graphic), et de la placer
   dans le répertoire "saved" de Tux Paint. C'est là où on les trouve (par
   défaut) :

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

   💡 Remarque : C'est également à partir de ce dossier que vous pouvez copier
   ou ouvrir des images dessinées dans Tux Paint à l'aide d'autres
   applications, ainsi l'option 'Export' de la boîte de dialogue 'Ouvrir' de
   Tux Paint peut être utilisée pour les copier dans un endroit plus facile
   d'accès plus sûr.

Utilisation du script d'importation "tuxpaint-import"

     Les utilisateurs Linux et Unix peuvent utiliser script shell
     "tuxpaint-import" qui s'installe lorsque vous installez Tux Paint. Il
     utilise certains outils NetPBM pour convertir l'image ("anytopnm"), la
     redimensionner pour qu'elle tienne dans le canevas de Tux Paint
     ("pnmscale") et la convertir en PNG ("pnmtopng").

     Il utilise également la commande "date" pour obtenir l'heure et la date
     actuelles, qui sont la convention de dénomination des fichiers utilisée
     par Tux Paint pour les fichiers enregistrés. (N'oubliez pas que vous
     n'êtes jamais invité à entrer un "nom de fichier " lorsque vous allez
     enregistrer ou ouvrir des images !)

     Pour utiliser ce script, exécutez simplement la commande à partir d'une
     invite de ligne de commande et indiquez-lui le (s) nom (s) du ou des
     fichiers que vous voulez convertir.

     They will be converted and placed in your Tux Paint "saved" directory.

     💡 Note: If you're doing this for a different user (e.g., your child)
     you'll need to make sure to run the command under their account.)

     Exemple :

       $ tuxpaint-import grandma.jpg
       grandma.jpg -> /home/username/.tuxpaint/saved/20211231012359.png
       jpegtopnm: WRITING A PPM FILE

     La première ligne ("tuxpaint-import grandma.jpg") est la commande à
     exécuter. Les deux lignes suivantes sont les sorties du programme
     pendant qu'il fonctionne.

     Vous pouvez maintenant charger Tux Paint, et une version de cette image
     originale sera disponible dans la boîte de dialogue «Ouvrir».
     Double-cliquez simplement sur son icône !

Importer des images manuellement

     Les utilisateurs de Windows, macOS et Haiku désirant importer des images
     dans Tux Paint doivent le faire manuellement.

     Chargez un programme graphique capable à la fois de charger votre image
     et d'enregistrer un fichier au format PNG. (Voir le fichier de
     documentation "PNG.html" pour une liste des logiciels suggérés et
     d'autres références.)

     Lorsque Tux Paint charge une image qui n'a pas la même taille que son
     canevas de dessin, il met à l'échelle (et parfois en coloriant les
     bords) l'image pour qu'elle tienne dans le canevas.

     Pour éviter que l'image ne soit étirée ou maculée, vous pouvez la
     redimensionner à la taille de la toile de Tux Paint. Cette taille dépend
     de la taille de la fenêtre Tux Paint, ou de la résolution à laquelle Tux
     Paint est exécuté, si il est en plein écran. (Remarque : la résolution
     par défaut est de 800x600.) Voir "Calculer les dimensions des images"
     ci-dessous.

     Sauvegarder l'image au format PNG. Il est fortement recommandé que vous
     nommiez le fichier en utilisant la date et l'heure courante, puisque
     c'est ce que Tux Paint utilise :

       YYYYMMDDhhmmss.png

       * YYYY = Year
       * MM = Mois (deux chiffres, "01"-"12")
       * DD = Jour du mois (deux chiffres, "01"-"31")
       * HH = Heure (deux chiffres,au format 24h, "00"-"23")
       * mm = Minute (deux chiffres, "00"-"59")
       * ss = Secondes (deux chiffres, "00"-"59")

     Exemple: "20210731110500.png",pour le 31 juillet 2021 à 11:05 du matin.

     Mettez cd fichier PNG dans votre répertoire Tux Paint "saved". (Voir
     ci-dessus)

  Calculer les dimensions des images

       Cette partie de documentation doit être réécrite puisque la nouvelle
       option "buttonsize" a été ajoutée. Pour l'heure, essayer de dessiner
       et de sauvegarder l'image dans Tux Paint, et ensuite déterminer quelle
       taille (largeur et hauteur en pixels) elle possède, et essayez de
       l'adapter lors de la mise à l'échelle en l'important dans Tux Paint.

                            Lectures complémentaires

   Les autres documents inclus avec Tux Paint (dans le répertoire "docs")
   incluent :

   Using Tux Paint:
              * OPTIONS.html
                Instructions détaillées sur les options en ligne de commande
                et les fichiers de configuration, pour ceux qui ne veulent
                pas utiliser l'outil Tux Paint Config.
              * Documentation sur l'outil 'Magie' ("magic-docs")
                Documentation pour chacun des outils "Magic" actuellement
                installés.

   How to extend Tux Paint:
              * EXTENDING.html
                Des instructions détaillées sur la création de pinceaux, de
                tampons, d'images de démarrage et de modèles; et l'ajout de
                polices; et créer un nouveau clavier virtuel et des méthodes
                de saisie.
              * PNG.html
                Remarques sur la création d'images bitmap au format PNG à
                utiliser dans Tux Paint.
              * SVG.html
                Remarques sur la création d'images vectorielles au format SVG
                à utiliser dans Tux Paint.

   Technical information:
              * INSTALL.html
                Instructions pour compiler et installer Tux Paint, le cas
                échéant.
              * SIGNALS.html
                Informations sur les signaux POSIX auxquels répond Tux Paint.

   Development history and license:
              * AUTHORS.txt
                Liste des auteurs et contributeurs.
              * CHANGES.txt
                Résumé des changements entre chaque version de Tux Paint.
              * COPYING.txt
                Tux Paint's software license, the GNU General Public License
                (GPL)

                           Comment obtenir de l'aide

   If you need help, there are numerous ways to interact with Tux Paint
   developers and other users:
     * Mentionner des bogues, ou demander de nouvelles fonctionnalités via le
       système de suivi des bogues
     * Participer aux nombreuses listes de diffusion de Tux Paint
     * Contacter les développeurs directement

   Pour en savoir plus, visitez la page "Contact" du site officiel de Tux
   Paint : https://tuxpaint.org/contact/

                               Comment participer

   Tux Paint is a volunteer-driven project, and we're happy to accept your
   help in a variety of ways:
     * Traduire Tux Paint dans une autre langue
     * Améliorer les traductions existantes
     * Créer des oeuvres (tampons, images de démarrage, modèles, pinceaux)
     * Ajouter ou améliorer des caractéristiques ou bien des outils "Magie"
     * Créer un programme d'étude en classe
     * Promouvoir ou aider ceux qui utilisent Tux Paint

   Pour en savoir plus, visitez la page "Nous aider" du site officiel de Tux
   Paint : https://tuxpaint.org/help/

                               Trademark notices

     * "Linux" is a registered trademark of Linus Torvalds.
     * "Microsoft" and "Windows" are registered trademarks of Microsoft Corp.
     * "Apple" and "macOS" are registered trademarks of Apple Inc.
     * "Twitter" is a registered trademark of Twitter, Inc.
     * "Tumblr" is a registered trademark of Tumblr, Inc.
